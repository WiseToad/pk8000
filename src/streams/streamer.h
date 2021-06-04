#ifndef STREAMER_H
#define STREAMER_H

#include "logging.h"
#include "streams.h"

template <typename StreamT>
class Streamer final:
        public Logger
{
public:
    explicit Streamer(StreamT * stream):
        stream_m(stream), isValid_m(true)
    {
        auto * logger = dynamic_cast<Logger *>(stream);
        if(logger != nullptr) {
            logger->captureLog(this);
        }
    }

    template <typename T>
    auto read(T * buf, size_t count, bool exact = true) -> size_t
    {
        if(!isValid()) {
            return 0;
        }

        count = stream_m->read(reinterpret_cast<uint8_t *>(buf), unitsToBytes<T>(count), exact);
        return bytesToUnits<T>(count);
    }

    auto isEof() const -> bool
    {
        return (stream_m == nullptr || stream_m->isEof());
    }

    template <typename T>
    void write(const T * buf, size_t count)
    {
        if(!isValid()) {
            return;
        }

        stream_m->write(reinterpret_cast<const uint8_t *>(buf), unitsToBytes<T>(count));
    }

    void flush()
    {
        if(!isValid()) {
            return;
        }

        stream_m->flush();
    }

    void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin)
    {
        seek<uint8_t>(pos, origin);
    }

    template <typename T>
    void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin)
    {
        if(!isValid()) {
            return;
        }

        stream_m->seek(unitsToBytes<T>(pos), origin);
    }

    auto pos() -> size_t
    {
        return pos<uint8_t>();
    }

    template <typename T>
    auto pos() -> size_t
    {
        return (stream_m ? bytesToUnits<T>(stream_m->pos()) : 0);
    }

    auto isValid() const -> bool
    {
        return (stream_m != nullptr && isValid_m && stream_m->isValid());
    }

    void invalidate()
    {
        isValid_m = false;
    }

    void recover()
    {
        if(stream_m) {
            stream_m->recover();
        }
        isValid_m = true;
    }

private:
    template <typename T>
    auto bytesToUnits(size_t bytes) -> size_t
    {
        if(bytes % sizeof(T) != 0) {
            msg(LogLevel::error, "Stream position and/or streamed size is out of sync");
            isValid_m = false;
        }
        return (bytes / sizeof(T));
    }

    template <typename T>
    auto unitsToBytes(size_t units) const
    {
        return (units * sizeof(T));
    }

    StreamT * stream_m;
    bool isValid_m;
};

#endif // STREAMER_H
