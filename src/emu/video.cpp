#include "video.h"
#include "bytes.h"
#include <vector>
#include <cstring>

// TODO: Scanline-based rendering

namespace {

template <PixelFormat>
struct PixelImpl;

template <>
struct PixelImpl<PixelFormat::xrgb8888> final
{
    using Type = uint32_t;
};

template <>
struct PixelImpl<PixelFormat::rgb565> final
{
    using Type = uint16_t;
};

template <PixelFormat F>
using Pixel = typename PixelImpl<F>::Type;

auto xrgb8888(uint8_t r, uint8_t g, uint8_t b) -> Pixel<PixelFormat::xrgb8888>
{
    return bytepack(0, r, g, b);
}

auto rgb565(uint8_t r, uint8_t g, uint8_t b) -> Pixel<PixelFormat::rgb565>
{
    return uint16_t(uint16_t(r & 0x1f) << 11) |
           uint16_t(uint16_t(g & 0x3f) << 5) |
           uint16_t(b & 0x1f);
}

template <PixelFormat F>
using Palette = std::array<Pixel<F>, 16>;

template <PixelFormat F>
Palette<F> defaultPalette;

template <>
Palette<PixelFormat::xrgb8888> defaultPalette<PixelFormat::xrgb8888> =
{
    xrgb8888(0x00, 0x00, 0x00),
    xrgb8888(0x00, 0x00, 0x00),
    xrgb8888(0x00, 0xaa, 0x00),
    xrgb8888(0x55, 0xff, 0x55),

    xrgb8888(0x00, 0x00, 0xaa),
    xrgb8888(0x55, 0x55, 0xff),
    xrgb8888(0x00, 0xaa, 0xaa),
    xrgb8888(0x55, 0xff, 0xff),

    xrgb8888(0xaa, 0x00, 0x00),
    xrgb8888(0xff, 0x55, 0x55),
    xrgb8888(0xaa, 0xaa, 0x00),
    xrgb8888(0xff, 0xff, 0x55),

    xrgb8888(0xaa, 0x00, 0xaa),
    xrgb8888(0xff, 0x55, 0xff),
    xrgb8888(0xaa, 0xaa, 0xaa),
    xrgb8888(0xff, 0xff, 0xff)
};

template <>
Palette<PixelFormat::rgb565> defaultPalette<PixelFormat::rgb565> =
{
    rgb565(0x00, 0x00, 0x00),
    rgb565(0x00, 0x00, 0x00),
    rgb565(0x00, 0x2a, 0x00),
    rgb565(0x0a, 0x3f, 0x0a),

    rgb565(0x00, 0x00, 0x15),
    rgb565(0x0a, 0x15, 0x1f),
    rgb565(0x00, 0x2a, 0x15),
    rgb565(0x0a, 0x3f, 0x1f),

    rgb565(0x15, 0x00, 0x00),
    rgb565(0x1f, 0x15, 0x0a),
    rgb565(0x15, 0x2a, 0x00),
    rgb565(0x1f, 0x3f, 0x0a),

    rgb565(0x15, 0x00, 0x15),
    rgb565(0x1f, 0x15, 0x1f),
    rgb565(0x15, 0x2a, 0x15),
    rgb565(0x1f, 0x3f, 0x1f)
};

class VideoRenderer:
        public Interface
{
public:
    template <PixelFormat F>
    static auto create(Memory * memory, FrameBuffer * frameBuffer,
                       const Palette<F> * palette = &defaultPalette<F>)
                       -> std::unique_ptr<VideoRenderer>
    {
        return std::make_unique<Impl<F>>(memory, frameBuffer, palette);
    }

    virtual void renderFrame() = 0;

private:
    template <PixelFormat F>
    class Impl;

    explicit VideoRenderer() = default;
};

template<PixelFormat F>
class VideoRenderer::Impl final:
        public VideoRenderer
{
public:
    explicit Impl(Memory * memory, FrameBuffer * frameBuffer,
                  const Palette<F> * palette = &defaultPalette<F>):
        memBanks_m(memory->memBanks()), ioPorts_m(memory->ioPorts()),
        frameBuffer_m(frameBuffer), palette_m(palette)
    {}

    virtual void renderFrame() override
    {
        memset(frameBuffer_m->data(), 0, frameBuffer_m->size());

        // Blank off the screen
        if((ioPorts_m->port86 & 0x10) == 0) {
            return;
        }

        switch(ioPorts_m->port84 & 0x30) {
            case 0x20:
                renderScreen0();
                break;
            case 0x00:
                renderScreen1();
                break;
            case 0x10:
                renderScreen2();
                break;
        }
    }

    void renderScreen0()
    {
        uint16_t vram = uint16_t(uint16_t(ioPorts_m->port84 & 0xc0) << 8);
        const uint8_t * charBuf  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port90 & 0x0e) << 10));
        const uint8_t * charGen  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port91 & 0x0e) << 10));

        const Pixel<F> * palette = palette_m->data();
        Pixel<F> fcol = palette[ioPorts_m->port88 & 0x0f];
        Pixel<F> bcol = palette[ioPorts_m->port88 >> 4];

        auto * fbuf = reinterpret_cast<Pixel<F> *>(frameBuffer_m->data());

        // Render upper border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }

        // Render characters row by row
        const uint8_t * cbuf = charBuf;
        for(int i = 0; i < 24; ++i) {
            // Extract character generator bits for single character row
            uint8_t charGenXtr[40 * 8];
            uint8_t * cgen = charGenXtr;
            for(int i = 0; i < 40; ++i, ++cbuf, cgen += 8) {
                *reinterpret_cast<uint64_t *>(cgen) =
                        *reinterpret_cast<const uint64_t *>(charGen + (unsigned(*cbuf) << 3));
            }
            cbuf += (64 - 40);
            // Render character row scanline by scanline
            cgen = charGenXtr;
            for(int i = 0; i < 8; ++i) {
                // Left border pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                // Left margin pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                // Character pixels
                for(int i = 0; i < 40; ++i, cgen += 8) {
                    uint8_t bits = *cgen;
                    *fbuf++ = (bits & 0x80 ? fcol : bcol);
                    *fbuf++ = (bits & 0x40 ? fcol : bcol);
                    *fbuf++ = (bits & 0x20 ? fcol : bcol);
                    *fbuf++ = (bits & 0x10 ? fcol : bcol);
                    *fbuf++ = (bits & 0x08 ? fcol : bcol);
                    *fbuf++ = (bits & 0x04 ? fcol : bcol);
                }
                cgen = cgen - 40 * 8 + 1;
                // Right margin pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                // Right border pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
            }
        }

        // Render lower border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }
    }

    void renderScreen1()
    {
        uint16_t vram = uint16_t(uint16_t(ioPorts_m->port84 & 0xc0) << 8);
        const uint8_t * charBuf  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port90 & 0x0f) << 10));
        const uint8_t * charGen  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port91 & 0x0e) << 10));
        const uint8_t * colorMap = charBuf + 0x400; // TODO: here is the quick hack now, prove it or find the proper way!

        const Pixel<F> * palette = palette_m->data();
        Pixel<F> bcol = palette[ioPorts_m->port88 >> 4];

        auto * fbuf = reinterpret_cast<Pixel<F> *>(frameBuffer_m->data());

        // Render upper border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }

        // Render characters row by row
        const uint8_t * cbuf = charBuf;
        for(int i = 0; i < 24; ++i) {
            // Extract characrer generator bits for single character row
            // Extract and convert color map for the same character row
            uint8_t charGenXtr[32 * 8];
            uint8_t * cgen = charGenXtr;
            Pixel<F> fore[32], back[32];
            for(int i = 0; i < 32; ++i, ++cbuf, cgen += 8) {
                // Character generator
                *reinterpret_cast<uint64_t *>(cgen) =
                        *reinterpret_cast<const uint64_t *>(charGen + (unsigned(*cbuf) << 3));
                // Color map
                uint8_t col = colorMap[*cbuf >> 3];
                fore[i] = palette[col & 0x0f];
                back[i] = palette[col >> 4];
            }
            // Render character row scanline by scanline
            cgen = charGenXtr;
            for(int i = 0; i < 8; ++i) {
                // Left border pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                // Character pixels
                for(int i = 0; i < 32; ++i, cgen += 8) {
                    uint8_t bits = *cgen;
                    Pixel<F> fcol = fore[i];
                    Pixel<F> bcol = back[i];
                    *fbuf++ = (bits & 0x80 ? fcol : bcol);
                    *fbuf++ = (bits & 0x40 ? fcol : bcol);
                    *fbuf++ = (bits & 0x20 ? fcol : bcol);
                    *fbuf++ = (bits & 0x10 ? fcol : bcol);
                    *fbuf++ = (bits & 0x08 ? fcol : bcol);
                    *fbuf++ = (bits & 0x04 ? fcol : bcol);
                    *fbuf++ = (bits & 0x02 ? fcol : bcol);
                    *fbuf++ = (bits & 0x01 ? fcol : bcol);
                }
                cgen = cgen - 32 * 8 + 1;
                // Right border pixels
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
                *fbuf++ = bcol;
            }
        }

        // Render lower border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }
    }

    void renderScreen2()
    {
        uint16_t vram = uint16_t(uint16_t(ioPorts_m->port84 & 0xc0) << 8);
        const uint8_t * charBuf  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port91 & 0x0e) << 10));
        const uint8_t * charGen  = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port93 & 0x08) << 10));
        const uint8_t * colorMap = memBanks_m->ram.data() + (vram | (uint16_t(ioPorts_m->port92 & 0x08) << 10));

        const Pixel<F> * palette = palette_m->data();
        Pixel<F> bcol = palette[ioPorts_m->port88 >> 4];

        auto * fbuf = reinterpret_cast<Pixel<F> *>(frameBuffer_m->data());

        // Render upper border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }

        // Render three screen blocks from top to bottom
        const uint8_t * cbuf = charBuf;
        for(int i = 0; i < 3; ++i) {
            for(int i = 0; i < 8; ++i) {
                // Extract characrer generator bits for single character row
                // Extract color map for the same character row
                uint8_t charGenXtr[32 * 8];
                uint8_t * cgen = charGenXtr;
                uint8_t colorMapXtr[32 * 8];
                uint8_t * cmap = colorMapXtr;
                for(int i = 0; i < 32; ++i, ++cbuf, cgen += 8, cmap += 8) {
                    // Character generator
                    *reinterpret_cast<uint64_t *>(cgen) =
                            *reinterpret_cast<const uint64_t *>(charGen + (unsigned(*cbuf) << 3));
                    // Color map
                    *reinterpret_cast<uint64_t *>(cmap) =
                            *reinterpret_cast<const uint64_t *>(colorMap + (unsigned(*cbuf) << 3));
                }
                // Render character row scanline by scanline
                cgen = charGenXtr;
                cmap = colorMapXtr;
                for(int i = 0; i < 8; ++i) {
                    // Left border pixels
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    // Character pixels
                    for(int i = 0; i < 32; ++i, cgen += 8, cmap += 8) {
                        uint8_t bits = *cgen;
                        uint8_t col = *cmap;
                        Pixel<F> fcol = palette[col & 0x0f];
                        Pixel<F> bcol = palette[col >> 4];
                        *fbuf++ = (bits & 0x80 ? fcol : bcol);
                        *fbuf++ = (bits & 0x40 ? fcol : bcol);
                        *fbuf++ = (bits & 0x20 ? fcol : bcol);
                        *fbuf++ = (bits & 0x10 ? fcol : bcol);
                        *fbuf++ = (bits & 0x08 ? fcol : bcol);
                        *fbuf++ = (bits & 0x04 ? fcol : bcol);
                        *fbuf++ = (bits & 0x02 ? fcol : bcol);
                        *fbuf++ = (bits & 0x01 ? fcol : bcol);
                    }
                    cgen = cgen - 32 * 8 + 1;
                    cmap = cmap - 32 * 8 + 1;
                    // Right border pixels
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                    *fbuf++ = bcol;
                }
            }
            charGen += 0x800;
            colorMap += 0x800;
        }

        // Render lower border
        for(int i = 0; i < 266 * 4; ++i) {
            *fbuf++ = bcol;
        }
    }

private:
    MemBanks * memBanks_m;
    IoPorts * ioPorts_m;
    FrameBuffer * frameBuffer_m;
    const Palette<F> * palette_m;
};

} // namespace

class FrameBuffer::Impl
{
public:
    explicit Impl(unsigned width, unsigned height, size_t pitch):
        width_m(width), height_m(height), pitch_m(pitch), data_m(pitch_m * height_m)
    {}

    unsigned width_m;
    unsigned height_m;
    size_t pitch_m;
    std::vector<uint8_t> data_m;
};

FrameBuffer::FrameBuffer(unsigned width, unsigned height, size_t pitch):
    impl(std::make_unique<Impl>(width, height, pitch))
{
}

FrameBuffer::~FrameBuffer()
{
}

auto FrameBuffer::width() const -> unsigned
{
    return impl->width_m;
}

auto FrameBuffer::height() const -> unsigned
{
    return impl->height_m;
}

auto FrameBuffer::pitch() const -> size_t
{
    return impl->pitch_m;
}

auto FrameBuffer::size() const -> size_t
{
    return impl->data_m.size();
}

auto FrameBuffer::data() -> uint8_t *
{
    return impl->data_m.data();
}

class Video::Impl final:
        public Video
{
public:
    explicit Impl(Memory * memory):
        memory_m(memory)
    {}

    virtual void init() override
    {
    }

    virtual void reset() override
    {
    }

    virtual void close() override
    {
    }

    virtual void startFrame() override
    {
    }

    virtual void renderFrame() override
    {
        if(renderer_m) {
            renderer_m->renderFrame();
        }
    }

    virtual void endFrame() override
    {
    }

    virtual void setPixelFormat(PixelFormat pixelFormat) override
    {
        switch(pixelFormat) {
            case PixelFormat::xrgb8888:
            default:
                setPixelFormat<PixelFormat::xrgb8888>();
                break;
            case PixelFormat::rgb565:
                setPixelFormat<PixelFormat::rgb565>();
                break;
        }
    }

    virtual auto frameBuffer() -> FrameBuffer * override
    {
        return frameBuffer_m.get();
    }

private:
    template <PixelFormat F>
    void setPixelFormat()
    {
        frameBuffer_m = std::make_unique<FrameBuffer>(videoWidth, videoHeight, videoWidth * sizeof(Pixel<F>));
        renderer_m = VideoRenderer::create<F>(memory_m, frameBuffer_m.get());
    }

    Memory * memory_m;
    std::unique_ptr<FrameBuffer> frameBuffer_m;
    std::unique_ptr<VideoRenderer> renderer_m;
};

auto Video::create(Memory * memory) -> std::unique_ptr<Video>
{
    return std::make_unique<Impl>(memory);
}
