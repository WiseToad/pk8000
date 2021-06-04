#include "compressor.h"

class Compressor::Impl final:
        public Compressor
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
        while(source->count() > 0 && target->count() > 0) {
            int32_t s = *source->data();
            int32_t t;
            if(s >= 0) {    // positive half-wave
                t = s & 16383;                      // target = source % 0.5
                t = (65535 - t) * t / 16384 / 3;    // target = (2 - target) * target * 2 / 3
                while(s >= 16384) {                 // while(source >= 0.5) {
                    t = (t + 32768) / 2;            //     target = (target + 1) * 0.5
                    s -= 16384;                     //     source -= 0.5
                }                                   // }
            } else {        // negative half-wave
                t = s | ~16383;                     // target = source % 0.5
                t = (65536 + t) * t / 16384 / 3;    // target = (2 + target) * target * 2 / 3
                while(s < -16384) {                 // while(source < -0.5) {
                    t = (t - 32768) / 2;            //     target = (target - 1) * 0.5
                    s += 16384;                     //     source += 0.5
                }                                   // }
            }
            *target->data() = t;

            source->advance();
            target->advance();
        }
    }
};

auto Compressor::create() -> std::unique_ptr<Compressor>
{
    return std::make_unique<Impl>();
}
