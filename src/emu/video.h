#ifndef VIDEO_H
#define VIDEO_H

#include "memory.h"

constexpr unsigned videoWidth = 266;
constexpr unsigned videoHeight = 200;
constexpr unsigned videoFps = 50;

enum class PixelFormat {
    xrgb8888, rgb565
};

class FrameBuffer final
{
public:
    explicit FrameBuffer(unsigned width, unsigned height, size_t pitch);
    ~FrameBuffer();

    auto width() const -> unsigned;
    auto height() const -> unsigned;
    auto pitch() const -> size_t;
    auto size() const -> size_t;
    auto data() -> uint8_t *;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

class Video:
        public ITimelineSubSystem,
        public Logger
{
public:
    static auto create(Memory * memory) -> std::unique_ptr<Video>;

    virtual void setPixelFormat(PixelFormat pixelFormat) = 0;
    virtual auto frameBuffer() -> FrameBuffer * = 0;

private:
    class Impl;
    explicit Video() = default;
};

#endif // VIDEO_H
