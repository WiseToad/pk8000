#include "wav.h"
#include "file.h"
#include "streamer.h"
#include "bytes.h"
#include <cstring>

namespace {

struct WavHeader final
{
    static constexpr uint8_t  riffHeaderSignature[] = { 'R', 'I', 'F', 'F' };
    static constexpr uint8_t  riffHeaderFormat[]    = { 'W', 'A', 'V', 'E' };
    static constexpr uint8_t  fmtHeaderSignature[]  = { 'f', 'm', 't', ' ' };
    static constexpr uint8_t  dataHeaderSignature[] = { 'd', 'a', 't', 'a' };
    static constexpr uint16_t fmtAudioFormat        = 1;

    struct RiffHeader final
    {
        uint8_t  signature[4];
        uint32_t size;
        uint8_t  format[4];
    };

    struct FmtHeader final
    {
        uint8_t  signature[4];
        uint32_t size;
    };

    struct Fmt final
    {
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };

    struct DataHeader final
    {
        uint8_t  signature[4];
        uint32_t size;
    };

    void init(const WavParams & params)
    {
        // Init riffHeader
        memcpy(riffHeader.signature, riffHeaderSignature, sizeof(riffHeaderSignature));
        riffHeader.size = sizeof(riffHeader.format) + sizeof(fmtHeader) + sizeof(fmt) + sizeof(dataHeader);
        memcpy(riffHeader.format, riffHeaderFormat, sizeof(riffHeaderFormat));

        // Init fmtHeader
        memcpy(fmtHeader.signature, fmtHeaderSignature, sizeof(fmtHeaderSignature));
        fmtHeader.size = sizeof(fmt);

        // Init fmt
        fmt.audioFormat   = fmtAudioFormat;
        fmt.numChannels   = uint16_t(params.numChannels);
        fmt.sampleRate    = uint32_t(params.sampleRate);
        fmt.bitsPerSample = uint16_t(params.bitsPerSample);
        fmt.blockAlign    = uint16_t(params.numChannels * (params.bitsPerSample / 8));
        fmt.byteRate      = uint32_t(params.sampleRate * fmt.blockAlign);

        // Init dataHeader
        memcpy(dataHeader.signature, dataHeaderSignature, sizeof(dataHeaderSignature));
        dataHeader.size = 0;
    }

    void read(Streamer<FileReader> * streamer)
    {
        // Read riffHeader
        streamer->read(&riffHeader, 1);
        if(!streamer->isValid()) {
            return;
        }
        if(memcmp(riffHeader.signature, riffHeaderSignature, sizeof(riffHeaderSignature)) ||
                memcmp(riffHeader.format, riffHeaderFormat, sizeof(riffHeaderFormat)))
        {
            streamer->msg(LogLevel::error, "Invalid WAV file RIFF header");
            streamer->invalidate();
            return;
        }
    #if BYTE_ORDER == BIG_ENDIAN
        riffHeader.size = byteswap(riffHeader.size);
    #endif

        // Read fmtHeader
        streamer->read(&fmtHeader, 1);
        if(!streamer->isValid()) {
            return;
        }
        if(memcmp(fmtHeader.signature, fmtHeaderSignature, sizeof(fmtHeaderSignature)) ||
                fmtHeader.size != sizeof(fmt))
        {
            streamer->msg(LogLevel::error, "Invalid WAV file fmt header");
            streamer->invalidate();
            return;
        }
    #if BYTE_ORDER == BIG_ENDIAN
        fmtHeader.size = byteswap(fmtHeader.size);
    #endif

        // Read fmt
        streamer->read(&fmt, 1);
        if(!streamer->isValid()) {
            return;
        }
    #if BYTE_ORDER == BIG_ENDIAN
        fmt.audioFormat   = byteswap(fmt.audioFormat);
        fmt.numChannels   = byteswap(fmt.numChannels);
        fmt.sampleRate    = byteswap(fmt.sampleRate);
        fmt.byteRate      = byteswap(fmt.byteRate);
        fmt.blockAlign    = byteswap(fmt.blockAlign);
        fmt.bitsPerSample = byteswap(fmt.bitsPerSample);
    #endif
        if(fmt.audioFormat != fmtAudioFormat) {
            streamer->msg(LogLevel::error, "Unsupported WAV file audio format");
            streamer->invalidate();
            return;
        }
        if(fmt.numChannels == 0 || fmt.numChannels > wavMaxNumChannels ||
                fmt.sampleRate == 0 || fmt.sampleRate > wavMaxSampleRate ||
                !(fmt.bitsPerSample == 8 || fmt.bitsPerSample == 16))
        {
            streamer->msg(LogLevel::error,
                          "Can not read WAV file with unsupported data format: "
                          "%d channels, %d bits, %d Hz sample rate",
                          fmt.numChannels, fmt.bitsPerSample, fmt.sampleRate);
            streamer->invalidate();
            return;
        }

        uint16_t blockAlign = fmt.numChannels * (fmt.bitsPerSample / 8);
        uint32_t byteRate = fmt.sampleRate * fmt.blockAlign;
        if(fmt.blockAlign != blockAlign || fmt.byteRate != byteRate) {
            streamer->msg(LogLevel::error, "Inconsistent WAV file fmt values");
            streamer->invalidate();
            return;
        }

        // Read dataHeader
        streamer->read(&dataHeader, 1);
        if(!streamer->isValid()) {
            return;
        }
        if(memcmp(dataHeader.signature, dataHeaderSignature, sizeof(dataHeaderSignature))) {
            streamer->msg(LogLevel::error, "Invalid WAV file data header");
            streamer->invalidate();
            return;
        }
    #if BYTE_ORDER == BIG_ENDIAN
        dataHeader.size = byteswap(dataHeader.size);
    #endif
    }

    void write(Streamer<FileWriter> * streamer)
    {
    #if BYTE_ORDER == BIG_ENDIAN
        WavHeader wavHeader = *this;
        wavHeader.riffHeader.size   = byteswap(wavHeader.riffHeader.size);
        wavHeader.fmtHeader.size    = byteswap(wavHeader.fmtHeader.size);
        wavHeader.fmt.audioFormat   = byteswap(wavHeader.fmt.audioFormat);
        wavHeader.fmt.numChannels   = byteswap(wavHeader.fmt.numChannels);
        wavHeader.fmt.sampleRate    = byteswap(wavHeader.fmt.sampleRate);
        wavHeader.fmt.byteRate      = byteswap(wavHeader.fmt.byteRate);
        wavHeader.fmt.blockAlign    = byteswap(wavHeader.fmt.blockAlign);
        wavHeader.fmt.bitsPerSample = byteswap(wavHeader.fmt.bitsPerSample);
        wavHeader.dataHeader.size   = byteswap(wavHeader.dataHeader.size);
    #else
        WavHeader & wavHeader = *this;
    #endif
        streamer->write(&wavHeader, 1);
    }

    RiffHeader riffHeader;
    FmtHeader fmtHeader;
    Fmt fmt;
    DataHeader dataHeader;
};

template <typename FileT>
class WavFileImpl:
        public Logger
{
public:
    explicit WavFileImpl():
        file_m(FileT::create()), isOpen_m(false)
    {
        captureLog(file_m.get());
        wavHeader_m.init({0, 0, 0});
    }

    auto params() const -> WavParams
    {
        return {int(wavHeader_m.fmt.numChannels),
                int(wavHeader_m.fmt.sampleRate),
                int(wavHeader_m.fmt.bitsPerSample)};
    }

    void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin)
    {
        if(!isValid()) {
            return;
        }
        if(origin == SeekOrigin::begin) {
            pos += sizeof(WavHeader);
        }
        file_m->seek(pos, origin);
        if(!file_m->isValid()) {
            return;
        }
        if(origin != SeekOrigin::begin && file_m->pos() < sizeof(WavHeader)) {
            file_m->seek(sizeof(WavHeader));
        }
    }

    auto pos() -> size_t
    {
        if(!isOpen_m) {
            return 0;
        }
        size_t pos = file_m->pos();
        if(pos < sizeof(WavHeader)) {
            isValid_m = false;
            return 0;
        }
        return (pos - sizeof(WavHeader));
    }

    auto isValid() const -> bool
    {
        return (isOpen_m && isValid_m && file_m->isValid());
    }

    void recover()
    {
        file_m->recover();
        isValid_m = true;
    }

protected:
    std::unique_ptr<FileT> file_m;
    WavHeader wavHeader_m;
    bool isOpen_m;
    bool isValid_m;
};

class WavFileReaderImpl final:
        public WavFileImpl<FileReader>
{
public:
    void open(const std::string & fileName)
    {
        close();

        file_m->open(fileName);
        if(!file_m->isValid()) {
            return;
        }

        Streamer<FileReader> streamer(file_m.get());
        wavHeader_m.read(&streamer);
        if(!streamer.isValid()) {
            return;
        }

        isOpen_m = true;
        isValid_m = true;
    }

    void close()
    {
        file_m->close();
        isOpen_m = false;
        wavHeader_m.init({0, 0, 0});
    }

    auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t
    {
        if(!isValid()) {
            return 0;
        }
        return file_m->read(buf, count, exact);
    }

    auto isEof() const -> bool
    {
        return (!isOpen_m || file_m->isEof());
    }
};

class WavFileWriterImpl final:
        public WavFileImpl<FileWriter>
{
public:
    void open(const std::string & fileName, bool trunc, const WavParams & params)
    {
        close();

        if(!trunc) {
            std::unique_ptr<FileReader> file = FileReader::create();
            captureLog(file.get());
            file->open(fileName);
            if(!file->isValid()) {
                trunc = true;
            } else {
                Streamer<FileReader> streamer(file.get());
                wavHeader_m.read(&streamer);
                if(!streamer.isValid()) {
                    return;
                }
            }
        }

        file_m->open(fileName, trunc);
        if(!file_m->isValid()) {
            return;
        }

        isOpen_m = true;
        isValid_m = true;

        writtenSize_m = 0;

        if(trunc) {
            wavHeader_m.init(params);

            Streamer<FileWriter> streamer(file_m.get());
            wavHeader_m.write(&streamer);
            if(!streamer.isValid()) {
                return;
            }
        }
    }

    void close()
    {
        updateWavHeader();

        file_m->close();
        isOpen_m = false;
        wavHeader_m.init({0, 0, 0});
    }

    void write(const uint8_t * buf, size_t count)
    {
        if(!isValid()) {
            return;
        }

        file_m->write(buf, count);
        if(!file_m->isValid()) {
            return;
        }

        writtenSize_m += count;
        if(writtenSize_m >= wavHeader_m.fmt.byteRate) {
            updateWavHeader();
        }
    }

    void flush()
    {
        updateWavHeader();

        if(!isValid()) {
            return;
        }
        file_m->flush();
    }

private:
    void updateWavHeader()
    {
        if(!isValid()) {
            return;
        }

        size_t pos = file_m->pos();
        file_m->seek(0, SeekOrigin::end);
        size_t size = file_m->pos();
        if(!file_m->isValid()) {
            return;
        }
        if(size < sizeof(WavHeader)) {
            isValid_m = false;
            return;
        }

        uint32_t dataSize = uint32_t(size - sizeof(WavHeader));
        if(dataSize != wavHeader_m.dataHeader.size) {
            wavHeader_m.dataHeader.size = uint32_t(size - sizeof(WavHeader));
            wavHeader_m.riffHeader.size = uint32_t(size - sizeof(WavHeader::RiffHeader::signature) -
                                                          sizeof(WavHeader::RiffHeader::size));
            file_m->seek(0, SeekOrigin::begin);
            Streamer<FileWriter> streamer(file_m.get());
            wavHeader_m.write(&streamer);
            if(!streamer.isValid()) {
                isValid_m = false;
                return;
            }
        }

        file_m->seek(pos, SeekOrigin::begin);

        writtenSize_m = 0;
    }

    size_t writtenSize_m;
};

} // namespace

class WavFileReader::Impl final:
        public WavFileReader
{
public:
    explicit Impl()
    {
        captureLog(&file_m);
    }

    virtual void open(const std::string & fileName) override
    {
        file_m.open(fileName);
    }

    virtual void close() override
    {
        file_m.close();
    }

    virtual auto params() const -> WavParams override
    {
        return file_m.params();
    }

    virtual auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t override
    {
        return file_m.read(buf, count, exact);
    }

    virtual auto isEof() const -> bool override
    {
        return file_m.isEof();
    }

    virtual void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin) override
    {
        file_m.seek(pos, origin);
    }

    virtual auto pos() -> size_t override
    {
        return file_m.pos();
    }

    virtual auto isValid() const -> bool override
    {
        return file_m.isValid();
    }

    virtual void recover() override
    {
        file_m.recover();
    }

private:
    WavFileReaderImpl file_m;
};

auto WavFileReader::create() -> std::unique_ptr<WavFileReader>
{
    return std::make_unique<Impl>();
}

class WavFileWriter::Impl final:
        public WavFileWriter
{
public:
    explicit Impl()
    {
        captureLog(&file_m);
    }

    virtual void open(const std::string & fileName, bool trunc, const WavParams & params) override
    {
        file_m.open(fileName, trunc, params);
    }

    virtual void close() override
    {
        file_m.close();
    }

    virtual auto params() const -> WavParams override
    {
        return file_m.params();
    }

    virtual void write(const uint8_t * buf, size_t count) override
    {
        file_m.write(buf, count);
    }

    virtual void flush() override
    {
        file_m.flush();
    }

    virtual void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin) override
    {
        file_m.seek(pos, origin);
    }

    virtual auto pos() -> size_t override
    {
        return file_m.pos();
    }

    virtual auto isValid() const -> bool override
    {
        return file_m.isValid();
    }

    virtual void recover() override
    {
        file_m.recover();
    }

private:
    WavFileWriterImpl file_m;
};

auto WavFileWriter::create() -> std::unique_ptr<WavFileWriter>
{
    return std::make_unique<Impl>();
}

class WavFileProber::Impl:
        public WavFileProber
{
public:
    virtual auto probe(FileReader * file) -> bool override
    {
        WavHeader::RiffHeader riffHeader;

        Streamer<FileReader> streamer(file);
        streamer.read(&riffHeader, 1);
        if(!streamer.isValid()) {
            return false;
        }
        if(memcmp(riffHeader.signature, WavHeader::riffHeaderSignature, sizeof(WavHeader::riffHeaderSignature)) ||
                memcmp(riffHeader.format, WavHeader::riffHeaderFormat, sizeof(WavHeader::riffHeaderFormat)))
        {
            return false;
        }
        return true;
    }
};

auto WavFileProber::create() -> std::unique_ptr<WavFileProber>
{
    return std::make_unique<Impl>();
}

class WavCvt::Impl final:
        public WavCvt
{
public:
    explicit Impl():
        sourceChannel_m(0), isValid_m(true)
    {}

    virtual void setSourceParams(const WavParams & params) override
    {
        if(!checkParams(params)) {
            msg(LogLevel::error, "Invalid source params specified for WAV converter");
            isValid_m = false;
            return;
        }
        if(sourceChannel_m >= params.numChannels) {
            msg(LogLevel::warn, "Source channel get out of bounds, forced to use channel 0");
            sourceChannel_m = 0;
        }
        sourceParams_m = params;
        isValid_m = true;
    }

    virtual void setTargetParams(const WavParams & params) override
    {
        if(!checkParams(params)) {
            msg(LogLevel::error, "Invalid target params specified for WAV converter");
            isValid_m = false;
            return;
        }
        if(params.numChannels != 1) {
            msg(LogLevel::error, "Only downmix to mono is supported for WAV converter");
            isValid_m = false;
            return;
        }
        targetParams_m = params;
        isValid_m = true;
    }

    virtual void setSourceChannel(int channel) override
    {
        if(channel < 0 || channel >= sourceParams_m.numChannels) {
            msg(LogLevel::error, "Invalid source channel specified for WAV converter");
            isValid_m = false;
            return;
        }
        sourceChannel_m = channel;
    }

    virtual void convert(Slice<const uint8_t> * source, Slice<uint8_t> * target) override
    {
        if(!isValid()) {
            return;
        }

        if(sourceParams_m.sampleRate != targetParams_m.sampleRate) {
            msg(LogLevel::error, "Resampling isn't supported for WAV converter");
            isValid_m = false;
            return;
        }

        convertSamples(source, target);
    }

    virtual auto isValid() const -> bool override
    {
        return isValid_m;
    }

    virtual void recover() override
    {
    }

private:
    auto checkParams(const WavParams & params) const -> bool
    {
        return (params.numChannels > 0 && params.numChannels <= wavMaxNumChannels &&
                params.sampleRate > 0 && params.sampleRate <= wavMaxSampleRate &&
                (params.bitsPerSample == 8 || params.bitsPerSample == 16));
    }

    void convertSamples(Slice<const uint8_t> * source, Slice<uint8_t> * target)
    {
        switch(sourceParams_m.bitsPerSample) {
            case 8:
                convertSamples<uint8_t>(source, target);
                break;
            case 16:
                convertSamples<int16_t>(source, target);
                break;
            default:
                msg(LogLevel::error, "Unsupported source sample type for WAV converter");
                isValid_m = false;
                break;
        }
    }

    template <typename SourceSampleT>
    void convertSamples(Slice<const uint8_t> * source, Slice<uint8_t> * target)
    {
        switch(targetParams_m.bitsPerSample) {
            case 8:
                convertSamples<SourceSampleT, uint8_t>(source, target);
                break;
            case 16:
                convertSamples<SourceSampleT, int16_t>(source, target);
                break;
            default:
                msg(LogLevel::error, "Unsupported target sample type for WAV converter");
                isValid_m = false;
                break;
        }
    }

    template <typename SourceSampleT, typename TargetSampleT>
    void convertSamples(Slice<const uint8_t> * source, Slice<uint8_t> * target)
    {
        auto s = reinterpret_cast<Slice<const SourceSampleT> *>(source);
        auto t = reinterpret_cast<Slice<TargetSampleT> *>(target);

        while(s->count() > 0 && t->count() > 0) {
            convertSample(s->data() + sourceChannel_m, t->data());
            s->advance(size_t(sourceParams_m.numChannels));
            t->advance(size_t(targetParams_m.numChannels));
        }
    }

    void convertSample(const uint8_t * source, uint8_t * target)
    {
        *target = *source;
    }

    void convertSample(const uint8_t * source, int16_t * target)
    {
        *target = int16_t(int16_t(*source - 0x80) << 8);
    }

    void convertSample(const int16_t * source, uint8_t * target)
    {
        *target = uint8_t(*source >> 8) + 0x80;
    }

    void convertSample(const int16_t * source, int16_t * target)
    {
        *target = *source;
    }

    WavParams sourceParams_m;
    WavParams targetParams_m;
    int sourceChannel_m;
    bool isValid_m;
};

auto WavCvt::create() -> std::unique_ptr<WavCvt>
{
    return std::make_unique<Impl>();
}
