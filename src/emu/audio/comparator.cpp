#include "comparator.h"

namespace {

constexpr int32_t triggerLevel = 8192;

constexpr std::array<int32_t, 5> edgeSamples = {
    -32767, -24576, 0, 24576, 32767
};

} // namespace

class Comparator::Impl final:
        public Comparator
{
public:
    virtual void convert(Slice<const uint8_t> * source, Slice<uint8_t> * target) override
    {
        convert(reinterpret_cast<Slice<const int32_t> *>(source),
                reinterpret_cast<Slice<int32_t> *>(target));
    }

    virtual auto isValid() const -> bool override
    {
        return true;
    }

    virtual void recover() override
    {
    }

private:
    void convert(Slice<const int32_t> * source, Slice<int32_t> * target)
    {
        do {
            if(isHighLevel_m) {
                while(source->count() > 0 && target->count() > 0) {
                    if(*source->data() < -triggerLevel) {
                        isHighLevel_m = false;
                        break;
                    }
                    if(edgeSamplePos_m < edgeSamples.size() - 1) {
                        ++edgeSamplePos_m;
                    }
                    *target->data() = edgeSamples[edgeSamplePos_m];
                    source->advance();
                    target->advance();
                }
            } else {
                while(source->count() > 0 && target->count() > 0) {
                    if(*source->data() > triggerLevel) {
                        isHighLevel_m = true;
                        break;
                    }
                    if(edgeSamplePos_m > 0) {
                        --edgeSamplePos_m;
                    }
                    *target->data() = edgeSamples[edgeSamplePos_m];
                    source->advance();
                    target->advance();
                }
            }
        } while(source->count() > 0 && target->count() > 0);
    }

    bool isHighLevel_m = false;
    size_t edgeSamplePos_m = 0;
};

auto Comparator::create() -> std::unique_ptr<Comparator>
{
    return std::make_unique<Impl>();
}
