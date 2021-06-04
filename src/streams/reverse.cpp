#include "reverse.h"

class ReverseReader::Impl final:
        public ReverseReader
{
public:
    explicit Impl():
        source_m(nullptr), isReverse_m(false)
    {}

    virtual void open(IRandomReader * source) override
    {
        source_m = source;
        isValid_m = true;
    }

    virtual void close() override
    {
        source_m = nullptr;
    }

    virtual void setReverse(bool isReverse) override
    {
        isReverse_m = isReverse;
    }

    virtual auto isReverse() const -> bool override
    {
        return isReverse_m;
    }

    virtual auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t override
    {
        if(!isValid()) {
            return 0;
        }

        if(!isReverse_m) {
            return source_m->read(buf, count, exact);
        }

        size_t readPos = source_m->pos();
        size_t readCount = std::min(count, readPos);
        readPos -= readCount;
        source_m->seek(readPos);
        source_m->read(buf, readCount);
        if(!source_m->isValid()) {
            return 0;
        }
        if(readCount < count && exact) {
            msg(LogLevel::error, "Could not read desired amount of data");
            isValid_m = false;
        }
        source_m->seek(readPos);

        return readCount;
    }

    virtual auto isEof() const -> bool override
    {
        if(!source_m) {
            return true;
        }
        if(!isReverse_m) {
            return source_m->isEof();
        }
        return (source_m->pos() == 0);
    }

    virtual auto isValid() const -> bool override
    {
        return (source_m != nullptr && isValid_m && source_m->isValid());
    }

    virtual void recover() override
    {
        if(source_m) {
            source_m->recover();
        }
        isValid_m = true;
    }

private:
    IRandomReader * source_m;
    bool isReverse_m;
    bool isValid_m;
};

auto ReverseReader::create() -> std::unique_ptr<ReverseReader>
{
    return std::make_unique<Impl>();
}
