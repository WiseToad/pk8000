#include "cvtstream.h"
#include <vector>

class CvtReader::Impl final:
        public CvtReader
{
public:
    explicit Impl(ICvt * converter, size_t bufSize):
        converter_m(converter), buf_m(bufSize), source_m(nullptr)
    {}

    virtual void open(ISequentalReader * source) override
    {
        source_m = source;
        avail_m = Slice<const uint8_t>(buf_m.data(), 0);
        isValid_m = true;
    }

    virtual void close() override
    {
        source_m = nullptr;
    }

    virtual auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t override
    {
        if(!isValid()) {
            return 0;
        }

        Slice<uint8_t> target(buf, count);
        while(target.count() > 0) {
            if(avail_m.count() == 0) {
                size_t actualCount = source_m->read(buf_m.data(), buf_m.size(), false);
                avail_m = Slice<const uint8_t>(buf_m.data(), actualCount);
                if(avail_m.count() == 0) {
                    break;
                }
            }
            converter_m->convert(&avail_m, &target);
            if(!converter_m->isValid() || (avail_m.count() > 0 && target.count() > 0)) {
                break;
            }
        }
        if(target.count() > 0 && exact) {
            msg(LogLevel::error, "Could not read desired amount of data");
            isValid_m = false;
        }
        return (count - target.count());
    }

    virtual auto isEof() const -> bool override
    {
        return (source_m == nullptr || (avail_m.count() == 0 && source_m->isEof()));
    }

    virtual auto isValid() const -> bool override
    {
        return (source_m != nullptr && isValid_m && converter_m->isValid() &&
                (avail_m.count() > 0 || source_m->isValid()));
    }

    virtual void recover() override
    {
        if(source_m) {
            source_m->recover();
        }
        converter_m->recover();
        isValid_m = true;
    }

private:
    ICvt * converter_m;
    std::vector<uint8_t> buf_m;
    Slice<const uint8_t> avail_m;
    ISequentalReader * source_m;
    bool isValid_m;
};

auto CvtReader::create(ICvt * converter, size_t bufSize) -> std::unique_ptr<CvtReader>
{
    return std::make_unique<Impl>(converter, bufSize);
}

// TODO: Implement the CvtWriter::Impl class as it became necessary for something
