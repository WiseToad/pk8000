#include "memory.h"

class Memory::Impl final:
        public Memory
{
public:
    virtual void init() override
    {
        memBanks_m.ram.fill(0);
        memBanks_m.rom.fill(0xff);
        memBanks_m.x1.fill(0xff);
        memBanks_m.x2.fill(0xff);

        reset();
    }

    virtual void reset() override
    {
        ioPorts_m.port80 = 0xfc;
        ioPorts_m.port81.fill(0xff);
        ioPorts_m.port82 = 0;
        ioPorts_m.port84 = 0x2f;
        ioPorts_m.port85 = 0;
        ioPorts_m.port86 = 0xce;
        ioPorts_m.port88 = 0;
        ioPorts_m.port8c = 0x00;
        ioPorts_m.port8d = 0x00;
        ioPorts_m.port90 = 0xf0;
        ioPorts_m.port91 = 0xf0;
        ioPorts_m.port92 = 0xf7;
        ioPorts_m.port93 = 0xf7;
    }

    virtual void close() override
    {
    }

    virtual auto memBanks() -> MemBanks * override
    {
        return &memBanks_m;
    }

    virtual auto ioPorts() -> IoPorts * override
    {
        return &ioPorts_m;
    }

    MemBanks memBanks_m;
    IoPorts ioPorts_m;
};

auto Memory::create() -> std::unique_ptr<Memory>
{
    return std::make_unique<Impl>();
}
