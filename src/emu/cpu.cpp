#include "cpu.h"
//#include "audio.h"

namespace {

const std::array<MemBankType, 4> readBankTypes = {
    MemBankType::rom,
    MemBankType::x1,
    MemBankType::x2,
    MemBankType::ram
};

const std::array<MemBankType, 4> writeBankTypes = {
    MemBankType::ram,
    MemBankType::ram,
    MemBankType::ram,
    MemBankType::ram
};

enum class RamMode {
    mode0, // screen 0 and undocumented screen
    mode1  // screen 1 and screen 2
};

class MemIo final
{
public:
    explicit MemIo(CpuRegs * cpuRegs, MemBanks * memBanks, IoPorts * ioPorts):
        cpuRegs_m(cpuRegs), memBanks_m(memBanks), ioPorts_m(ioPorts)
    {
        unsigned * p = ramClockBuf_m.data();
        for(size_t i = 0; i < 2; ++i) {
            *p++ = 3;
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }
        for(size_t i = 0; i < 40; ++i) {
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }
        for(size_t i = 0; i < 10; ++i) {
            *p++ = 3;
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }
        for(size_t i = 0; i < 40; ++i) {
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }
        for(size_t i = 0; i < 10; ++i) {
            *p++ = 3;
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }
        for(size_t i = 0; i < 20; ++i) {
            *p++ = 2;
            *p++ = 1;
            *p++ = 0;
        }

        unsigned ** pp = ramClock_m.data();
        for(size_t i = 0; i < 40; ++i) {
            *pp++ = &ramClockBuf_m[0];
            *pp++ = &ramClockBuf_m[96];
            *pp++ = &ramClockBuf_m[32];
            *pp++ = &ramClockBuf_m[128];
            *pp++ = &ramClockBuf_m[64];
        }
    }

    void init()
    {
        uint8_t port80 = ioPorts_m->port80;
        for(size_t i = 0; i < 4; ++i) {
            size_t bankPos = port80 & 0x03;
            readBankMap_m[i] = memBank(readBankTypes[bankPos])->data();
            writeBankMap_m[i] = memBank(writeBankTypes[bankPos])->data();
            port80 >>= 2;
        }

        ramMode_m = (ioPorts_m->port84 & 0x20 ? RamMode::mode0 : RamMode::mode1);
    }

    void peek(uint8_t * data, uint16_t addr)
    {
        *data = readBankMap_m[addr >> 14][addr];
    }

    void poke(uint16_t addr, uint8_t data)
    {
        writeBankMap_m[addr >> 14][addr] = data;
    }

    void read(uint8_t * data, uint16_t addr)
    {
        cpuRegs_m->clock += 3;
        size_t bankPos = addr >> 14;
        uint8_t * memBankData = readBankMap_m[bankPos];
        MemBankType memBankType = readBankTypes[bankPos];
        wait(MemAccessType::read, memBankType, addr);
        memHookTrigger_m.fire(MemAccessType::read, memBankType, addr);
        ++cpuRegs_m->clock;
        *data = memBankData[addr];
    }

    void read(uint16_t * data, uint16_t addr)
    {
        uint8_t w, z;
        read(&z, addr);
        read(&w, addr + 1);
        *data = bytepack(w, z);
    }

    void write(uint16_t addr, uint8_t data)
    {
        cpuRegs_m->clock += 4;
        size_t bankPos = addr >> 14;
        uint8_t * memBankData = writeBankMap_m[bankPos];
        MemBankType memBankType = writeBankTypes[bankPos];
        wait(MemAccessType::write, memBankType, addr);
        memHookTrigger_m.fire(MemAccessType::write, memBankType, addr);
        ++cpuRegs_m->clock;
        memBankData[addr] = data;
    }

    void write(uint16_t addr, uint16_t data)
    {
        write(addr, bytelo(data));
        write(addr + 1, bytehi(data));
    }

    void fetch(uint8_t * data)
    {
        read(data, cpuRegs_m->pc++);
    }

    void fetch(uint16_t * data)
    {
        uint8_t w, z;
        fetch(&z);
        fetch(&w);
        *data = bytepack(w, z);
    }

    void push(uint16_t data)
    {
        write(--cpuRegs_m->sp, bytehi(data));
        write(--cpuRegs_m->sp, bytelo(data));
    }

    void pop(uint16_t * data)
    {
        uint8_t w, z;
        read(&z, cpuRegs_m->sp++);
        read(&w, cpuRegs_m->sp++);
        *data = bytepack(w, z);
    }

    auto createMemHook(const MemHook::HookFunc & hookFunc) -> std::unique_ptr<MemHook>
    {
        return std::make_unique<MemHook>(&memHookTrigger_m, hookFunc);
    }

private:
    auto memBank(MemBankType memBankType) -> MemBank *
    {
        switch(memBankType) {
            case MemBankType::ram:
                return &memBanks_m->ram;
            case MemBankType::rom:
                return &memBanks_m->rom;
            case MemBankType::x1:
                return &memBanks_m->x1;
            case MemBankType::x2:
                return &memBanks_m->x2;
        }
        // TODO
        return nullptr;
    }

    void wait(MemAccessType memAccessType, MemBankType memBankType, uint16_t addr)
    {
        if(memBankType == MemBankType::ram) {
            if(ramMode_m == RamMode::mode0) {
                cpuRegs_m->clock += ramClock_m[cpuRegs_m->clock >> 8][cpuRegs_m->clock & 0xff];
            } else {
                cpuRegs_m->clock += 3 - (cpuRegs_m->clock & 3);
            }
        }
    }

    CpuRegs * cpuRegs_m;
    MemBanks * memBanks_m;
    IoPorts * ioPorts_m;

    std::array<uint8_t *, 4> readBankMap_m;
    std::array<uint8_t *, 4> writeBankMap_m;

    RamMode ramMode_m;
    std::array<unsigned, 388> ramClockBuf_m;
    std::array<unsigned *, 200> ramClock_m;

    MemHook::HookTrigger memHookTrigger_m;
};

} // namespace

class RetHook::Impl final
{
public:
    explicit Impl(CpuRegs * cpuRegs, HookTrigger * trigger, const HookFunc & hookFunc):
        cpuRegs_m(cpuRegs), hook_m(trigger, memFunc(this, &Impl::hookFunc)),
        hookFunc_m(hookFunc), isActive_m(false)
    {}

    void setHookFunc(const HookFunc & hookFunc)
    {
        hookFunc_m = hookFunc;
    }

    void activate(bool isActive = true)
    {
        isActive_m = isActive;
        if(isActive) {
            sp_m = cpuRegs_m->sp;
        }
    }

    void hookFunc()
    {
        if(isActive_m && cpuRegs_m->sp == sp_m) {
            isActive_m = false;
            hookFunc_m();
        }
    }

    CpuRegs * cpuRegs_m;

    Hook<> hook_m;
    HookFunc hookFunc_m;

    bool isActive_m;
    uint16_t sp_m;
};

RetHook::RetHook(CpuRegs * cpuRegs, HookTrigger * trigger, const HookFunc & hookFunc):
    impl(std::make_unique<Impl>(cpuRegs, trigger, hookFunc))
{
}

RetHook::~RetHook()
{
}

void RetHook::setHookFunc(const HookFunc & hookFunc)
{
    impl->setHookFunc(hookFunc);
}

void RetHook::activate(bool isActive)
{
    impl->activate(isActive);
}

auto RetHook::isActive() const -> bool
{
    return impl->isActive_m;
}

class Cpu::Impl final:
        public Cpu
{
public:
    explicit Impl(Memory * memory):
        memIo_m(&cpuRegs_m, memory->memBanks(), memory->ioPorts()),
        ioPorts_m(memory->ioPorts())
    {
        for(size_t i = 0; i < 256; ++i) {
            flags_m[i] = 2;

            if(i & 0x80) {
                flags_m[i] |= CpuFlag::s;
            }
            if(i == 0) {
                flags_m[i] |= CpuFlag::z;
            }

            size_t a = i;
            a ^= a >> 4;
            a ^= a >> 2;
            a ^= a >> 1;
            a &= 1;
            if(!a) {
                flags_m[i] |= CpuFlag::p;
            }
        }
    }

    virtual void init() override
    {
        reset();
    }

    virtual void reset() override
    {
        cpuRegs_m.state = 0;
        cpuRegs_m.clock = 0;
        cpuRegs_m.a  = 0;
        cpuRegs_m.f  = 2;
        cpuRegs_m.bc = 0;
        cpuRegs_m.de = 0;
        cpuRegs_m.hl = 0;
        cpuRegs_m.sp = 0;
        cpuRegs_m.pc = 0;
    }

    virtual void close() override
    {
    }

    virtual void startFrame() override
    {
        memIo_m.init();
    }

    virtual void renderFrame() override
    {
        if(cpuRegs_m.state & CpuState::inte) {
            cpuRegs_m.state &= ~CpuState::halt;
            rst7();
            intHookTrigger_m.fire();
        }

        if(cpuRegs_m.state & CpuState::halt) {
            cpuRegs_m.clock = clocksPerFrame;
        }

        while(cpuRegs_m.clock < clocksPerFrame) {
            opHookTrigger_m.fire();
            uint8_t op;
            memIo_m.fetch(&op);
            (this->*opFuncs[op])();
        }
    }

    virtual void endFrame() override
    {
        cpuRegs_m.clock -= clocksPerFrame;
    }

    virtual auto cpuRegs() -> CpuRegs * override
    {
        return &cpuRegs_m;
    }

    virtual void memPeek(uint8_t * data, uint16_t addr) override
    {
        memIo_m.peek(data, addr);
    }

    virtual void memPoke(uint16_t addr, uint8_t data) override
    {
        memIo_m.poke(addr, data);
    }

    virtual auto createMemHook(const MemHook::HookFunc & hookFunc) -> std::unique_ptr<MemHook> override
    {
        return memIo_m.createMemHook(hookFunc);
    }

    virtual auto createIntHook(const IntHook::HookFunc & hookFunc) -> std::unique_ptr<IntHook> override
    {
        return std::make_unique<IntHook>(&intHookTrigger_m, hookFunc);
    }

    virtual auto createOpHook (const OpHook::HookFunc  & hookFunc) -> std::unique_ptr<OpHook> override
    {
        return std::make_unique<OpHook>(&opHookTrigger_m, hookFunc);
    }

    virtual auto createRetHook(const RetHook::HookFunc & hookFunc) -> std::unique_ptr<RetHook> override
    {
        return std::make_unique<RetHook>(&cpuRegs_m, &retHookTrigger_m, hookFunc);
    }

private:
        //-- mov r, r --//

    void movRR(uint8_t * r1, uint8_t r2)
    {
        cpuRegs_m.clock += 2;
        *r1 = r2;
    }

        //-- mov a, r --//

    void movAA()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.a);
    }

    void movAB()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.b);
    }

    void movAC()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.c);
    }

    void movAD()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.d);
    }

    void movAE()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.e);
    }

    void movAH()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.h);
    }

    void movAL()
    {
        movRR(&cpuRegs_m.a, cpuRegs_m.l);
    }

        //-- mov b, r --//

    void movBA()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.a);
    }

    void movBB()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.b);
    }

    void movBC()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.c);
    }

    void movBD()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.d);
    }

    void movBE()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.e);
    }

    void movBH()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.h);
    }

    void movBL()
    {
        movRR(&cpuRegs_m.b, cpuRegs_m.l);
    }

        //-- mov c, r --//

    void movCA()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.a);
    }

    void movCB()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.b);
    }

    void movCC()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.c);
    }

    void movCD()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.d);
    }

    void movCE()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.e);
    }

    void movCH()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.h);
    }

    void movCL()
    {
        movRR(&cpuRegs_m.c, cpuRegs_m.l);
    }

        //-- mov d, r --//

    void movDA()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.a);
    }

    void movDB()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.b);
    }

    void movDC()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.c);
    }

    void movDD()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.d);
    }

    void movDE()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.e);
    }

    void movDH()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.h);
    }

    void movDL()
    {
        movRR(&cpuRegs_m.d, cpuRegs_m.l);
    }

        //-- mov e, r --//

    void movEA()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.a);
    }

    void movEB()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.b);
    }

    void movEC()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.c);
    }

    void movED()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.d);
    }

    void movEE()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.e);
    }

    void movEH()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.h);
    }

    void movEL()
    {
        movRR(&cpuRegs_m.e, cpuRegs_m.l);
    }

        //-- mov h, r --//

    void movHA()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.a);
    }

    void movHB()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.b);
    }

    void movHC()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.c);
    }

    void movHD()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.d);
    }

    void movHE()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.e);
    }

    void movHH()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.h);
    }

    void movHL()
    {
        movRR(&cpuRegs_m.h, cpuRegs_m.l);
    }

        //-- mov l, r --//

    void movLA()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.a);
    }

    void movLB()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.b);
    }

    void movLC()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.c);
    }

    void movLD()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.d);
    }

    void movLE()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.e);
    }

    void movLH()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.h);
    }

    void movLL()
    {
        movRR(&cpuRegs_m.l, cpuRegs_m.l);
    }

        //-- mov r, m --//

    void movRM(uint8_t * r)
    {
        ++cpuRegs_m.clock;
        memIo_m.read(r, cpuRegs_m.hl);
    }

    void movAM()
    {
        movRM(&cpuRegs_m.a);
    }

    void movBM()
    {
        movRM(&cpuRegs_m.b);
    }

    void movCM()
    {
        movRM(&cpuRegs_m.c);
    }

    void movDM()
    {
        movRM(&cpuRegs_m.d);
    }

    void movEM()
    {
        movRM(&cpuRegs_m.e);
    }

    void movHM()
    {
        movRM(&cpuRegs_m.h);
    }

    void movLM()
    {
        movRM(&cpuRegs_m.l);
    }

        //-- mov m, r --//

    inline void movMR(uint8_t r)
    {
        ++cpuRegs_m.clock;
        memIo_m.write(cpuRegs_m.hl, r);
    }

    void movMA()
    {
        movMR(cpuRegs_m.a);
    }

    void movMB()
    {
        movMR(cpuRegs_m.b);
    }

    void movMC()
    {
        movMR(cpuRegs_m.c);
    }

    void movMD()
    {
        movMR(cpuRegs_m.d);
    }

    void movME()
    {
        movMR(cpuRegs_m.e);
    }

    void movMH()
    {
        movMR(cpuRegs_m.h);
    }

    void movML()
    {
        movMR(cpuRegs_m.l);
    }

        //-- mvi r, data --//

    void mviR(uint8_t * r)
    {
        ++cpuRegs_m.clock;
        memIo_m.fetch(r);
    }

    void mviA()
    {
        mviR(&cpuRegs_m.a);
    }

    void mviB()
    {
        mviR(&cpuRegs_m.b);
    }

    void mviC()
    {
        mviR(&cpuRegs_m.c);
    }

    void mviD()
    {
        mviR(&cpuRegs_m.d);
    }

    void mviE()
    {
        mviR(&cpuRegs_m.e);
    }

    void mviH()
    {
        mviR(&cpuRegs_m.h);
    }

    void mviL()
    {
        mviR(&cpuRegs_m.l);
    }

        //-- mvi m, data --//

    void mviM()
    {
        ++cpuRegs_m.clock;
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        memIo_m.write(cpuRegs_m.hl, tmp);
    }

        //-- lxi rp, data --//

    void lxiRP(uint16_t * rp)
    {
        ++cpuRegs_m.clock;
        memIo_m.fetch(rp);
    }

    void lxiB()
    {
        lxiRP(&cpuRegs_m.bc);
    }

    void lxiD()
    {
        lxiRP(&cpuRegs_m.de);
    }

    void lxiH()
    {
        lxiRP(&cpuRegs_m.hl);
    }

    void lxiSP()
    {
        lxiRP(&cpuRegs_m.sp);
    }

        //-- lda/sta addr --//

    void lda()
    {
        ++cpuRegs_m.clock;
        uint16_t addr;
        memIo_m.fetch(&addr);
        memIo_m.read(&cpuRegs_m.a, addr);
    }

    void sta()
    {
        ++cpuRegs_m.clock;
        uint16_t addr;
        memIo_m.fetch(&addr);
        memIo_m.write(addr, cpuRegs_m.a);
    }

        //-- lhld/shld addr --//

    void lhld()
    {
        ++cpuRegs_m.clock;
        uint16_t addr;
        memIo_m.fetch(&addr);
        memIo_m.read(&cpuRegs_m.hl, addr);
    }

    void shld()
    {
        ++cpuRegs_m.clock;
        uint16_t addr;
        memIo_m.fetch(&addr);
        memIo_m.write(addr, cpuRegs_m.hl);
    }

        //-- ldax rp --//

    void ldaxRP(uint16_t rp)
    {
        ++cpuRegs_m.clock;
        memIo_m.read(&cpuRegs_m.a, rp);
    }

    void ldaxB()
    {
        ldaxRP(cpuRegs_m.bc);
    }

    void ldaxD()
    {
        ldaxRP(cpuRegs_m.de);
    }

        //-- stax rp --//

    void staxRP(uint16_t rp)
    {
        ++cpuRegs_m.clock;
        memIo_m.write(rp, cpuRegs_m.a);
    }

    void staxB()
    {
        staxRP(cpuRegs_m.bc);
    }

    void staxD()
    {
        staxRP(cpuRegs_m.de);
    }

        //-- xchg; sphl; xthl --//

    void xchg()
    {
        ++cpuRegs_m.clock;
        uint16_t tmp = cpuRegs_m.hl;
        cpuRegs_m.hl = cpuRegs_m.de;
        cpuRegs_m.de = tmp;
    }

    void sphl()
    {
        cpuRegs_m.clock += 2;
        cpuRegs_m.sp = cpuRegs_m.hl;
    }

    void xthl()
    {
        ++cpuRegs_m.clock;
        uint16_t tmp;
        memIo_m.pop(&tmp);
        memIo_m.push(cpuRegs_m.hl);
        cpuRegs_m.clock += 2;
        cpuRegs_m.hl = tmp;
    }

        //-- push rp --//

    void pushRP(uint16_t rp)
    {
        cpuRegs_m.clock += 2;
        memIo_m.push(rp);
    }

    void pushB()
    {
        pushRP(cpuRegs_m.bc);
    }

    void pushD()
    {
        pushRP(cpuRegs_m.de);
    }

    void pushH()
    {
        pushRP(cpuRegs_m.hl);
    }

    void pushPSW()
    {
        pushRP(cpuRegs_m.psw);
    }

        //-- pop rp --//

    void popRP(uint16_t * rp)
    {
        ++cpuRegs_m.clock;
        memIo_m.pop(rp);
    }

    void popB()
    {
        popRP(&cpuRegs_m.bc);
    }

    void popD()
    {
        popRP(&cpuRegs_m.de);
    }

    void popH()
    {
        popRP(&cpuRegs_m.hl);
    }

    void popPSW()
    {
        popRP(&cpuRegs_m.psw);
    }

        //-- add r/m; adi data --//

    void add(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = ((cpuRegs_m.a & ~0x10) + (data & ~0x10)) & 0x10;
        uint16_t tmp = uint16_t(cpuRegs_m.a) + uint16_t(data);
        cpuRegs_m.a = bytelo(tmp);
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | (bytehi(tmp) & 0x01) | ac;
    }

    void addA()
    {
        add(cpuRegs_m.a);
    }

    void addB()
    {
        add(cpuRegs_m.b);
    }

    void addC()
    {
        add(cpuRegs_m.c);
    }

    void addD()
    {
        add(cpuRegs_m.d);
    }

    void addE()
    {
        add(cpuRegs_m.e);
    }

    void addH()
    {
        add(cpuRegs_m.h);
    }

    void addL()
    {
        add(cpuRegs_m.l);
    }

    void addM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        add(tmp);
    }

    void adi()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        add(tmp);
    }

        //-- adc r/m; aci data --//

    void adc(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = ((cpuRegs_m.a & ~0x10) + (data & ~0x10) + (cpuRegs_m.f & 0x01)) & 0x10;
        uint16_t tmp = uint16_t(cpuRegs_m.a) + uint16_t(data) + uint16_t(cpuRegs_m.f & 0x01);
        cpuRegs_m.a = bytelo(tmp);
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | (bytehi(tmp) & 0x01) | ac;
    }

    void adcA()
    {
        adc(cpuRegs_m.a);
    }

    void adcB()
    {
        adc(cpuRegs_m.b);
    }

    void adcC()
    {
        adc(cpuRegs_m.c);
    }

    void adcD()
    {
        adc(cpuRegs_m.d);
    }

    void adcE()
    {
        adc(cpuRegs_m.e);
    }

    void adcH()
    {
        adc(cpuRegs_m.h);
    }

    void adcL()
    {
        adc(cpuRegs_m.l);
    }

    void adcM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        adc(tmp);
    }

    void aci()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        adc(tmp);
    }

        //-- sub r/m; sui data --//

    void sub(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = ((cpuRegs_m.a & ~0x10) - (data & ~0x10)) & 0x10;
        uint16_t tmp = uint16_t(cpuRegs_m.a) - uint16_t(data);
        cpuRegs_m.a = bytelo(tmp);
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | (bytehi(tmp) & 0x01) | ac;
    }

    void subA()
    {
        sub(cpuRegs_m.a);
    }

    void subB()
    {
        sub(cpuRegs_m.b);
    }

    void subC()
    {
        sub(cpuRegs_m.c);
    }

    void subD()
    {
        sub(cpuRegs_m.d);
    }

    void subE()
    {
        sub(cpuRegs_m.e);
    }

    void subH()
    {
        sub(cpuRegs_m.h);
    }

    void subL()
    {
        sub(cpuRegs_m.l);
    }

    void subM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        sub(tmp);
    }

    void sui()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        sub(tmp);
    }

        //-- sbb r/m; sbi data --//

    void sbb(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = ((cpuRegs_m.a & ~0x10) - (data & ~0x10) - (cpuRegs_m.f & 0x01)) & 0x10;
        uint16_t tmp = uint16_t(cpuRegs_m.a) - uint16_t(data) - uint16_t(cpuRegs_m.f & 0x01);
        cpuRegs_m.a = bytelo(tmp);
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | (bytehi(tmp) & 0x01) | ac;
    }

    void sbbA()
    {
        sbb(cpuRegs_m.a);
    }

    void sbbB()
    {
        sbb(cpuRegs_m.b);
    }

    void sbbC()
    {
        sbb(cpuRegs_m.c);
    }

    void sbbD()
    {
        sbb(cpuRegs_m.d);
    }

    void sbbE()
    {
        sbb(cpuRegs_m.e);
    }

    void sbbH()
    {
        sbb(cpuRegs_m.h);
    }

    void sbbL()
    {
        sbb(cpuRegs_m.l);
    }

    void sbbM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        sbb(tmp);
    }

    void sbi()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        sbb(tmp);
    }

        //-- ana r/m; ani data --//

    void ana(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = uint8_t(((cpuRegs_m.a & 0x08) | (data & 0x08)) << 1);
        cpuRegs_m.a &= data;
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | ac;
    }

    void anaA()
    {
        ana(cpuRegs_m.a);
    }

    void anaB()
    {
        ana(cpuRegs_m.b);
    }

    void anaC()
    {
        ana(cpuRegs_m.c);
    }

    void anaD()
    {
        ana(cpuRegs_m.d);
    }

    void anaE()
    {
        ana(cpuRegs_m.e);
    }

    void anaH()
    {
        ana(cpuRegs_m.h);
    }

    void anaL()
    {
        ana(cpuRegs_m.l);
    }

    void anaM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        ana(tmp);
    }

    void ani()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        ana(tmp);
    }

        //-- xra r/m; xri data --//

    void xra(uint8_t data)
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.a ^= data;
        cpuRegs_m.f = flags_m[cpuRegs_m.a];
    }

    void xraA()
    {
        xra(cpuRegs_m.a);
    }

    void xraB()
    {
        xra(cpuRegs_m.b);
    }

    void xraC()
    {
        xra(cpuRegs_m.c);
    }

    void xraD()
    {
        xra(cpuRegs_m.d);
    }

    void xraE()
    {
        xra(cpuRegs_m.e);
    }

    void xraH()
    {
        xra(cpuRegs_m.h);
    }

    void xraL()
    {
        xra(cpuRegs_m.l);
    }

    void xraM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        xra(tmp);
    }

    void xri()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        xra(tmp);
    }

        //-- ora r/m; ori data --//

    void ora(uint8_t data)
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.a |= data;
        cpuRegs_m.f = flags_m[cpuRegs_m.a];
    }

    void oraA()
    {
        ora(cpuRegs_m.a);
    }

    void oraB()
    {
        ora(cpuRegs_m.b);
    }

    void oraC()
    {
        ora(cpuRegs_m.c);
    }

    void oraD()
    {
        ora(cpuRegs_m.d);
    }

    void oraE()
    {
        ora(cpuRegs_m.e);
    }

    void oraH()
    {
        ora(cpuRegs_m.h);
    }

    void oraL()
    {
        ora(cpuRegs_m.l);
    }

    void oraM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        ora(tmp);
    }

    void ori()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        ora(tmp);
    }

        //-- cmp r/m; cpi data --//

    void cmp(uint8_t data)
    {
        ++cpuRegs_m.clock;
        uint8_t ac = ((cpuRegs_m.a & ~0x10) - (data & ~0x10)) & 0x10;
        uint16_t tmp = uint16_t(cpuRegs_m.a) - uint16_t(data);
        cpuRegs_m.f = flags_m[bytelo(tmp)] | (bytehi(tmp) & 0x01) | ac;
    }

    void cmpA()
    {
        cmp(cpuRegs_m.a);
    }

    void cmpB()
    {
        cmp(cpuRegs_m.b);
    }

    void cmpC()
    {
        cmp(cpuRegs_m.c);
    }

    void cmpD()
    {
        cmp(cpuRegs_m.d);
    }

    void cmpE()
    {
        cmp(cpuRegs_m.e);
    }

    void cmpH()
    {
        cmp(cpuRegs_m.h);
    }

    void cmpL()
    {
        cmp(cpuRegs_m.l);
    }

    void cmpM()
    {
        uint8_t tmp;
        memIo_m.read(&tmp, cpuRegs_m.hl);
        cmp(tmp);
    }

    void cpi()
    {
        uint8_t tmp;
        memIo_m.fetch(&tmp);
        cmp(tmp);
    }

        //-- inr r --//

    void inrR(uint8_t * r)
    {
        cpuRegs_m.clock += 2;
        uint8_t ac = ((*r & ~0x10) + 1) & 0x10;
        cpuRegs_m.f = flags_m[++(*r)] | (cpuRegs_m.f & 0x01) | ac;
    }

    void inrA()
    {
        inrR(&cpuRegs_m.a);
    }

    void inrB()
    {
        inrR(&cpuRegs_m.b);
    }

    void inrC()
    {
        inrR(&cpuRegs_m.c);
    }

    void inrD()
    {
        inrR(&cpuRegs_m.d);
    }

    void inrE()
    {
        inrR(&cpuRegs_m.e);
    }

    void inrH()
    {
        inrR(&cpuRegs_m.h);
    }

    void inrL()
    {
        inrR(&cpuRegs_m.l);
    }

        //-- inr m --//

    void inrM()
    {
        ++cpuRegs_m.clock;
        uint8_t r;
        memIo_m.read(&r, cpuRegs_m.hl);
        uint8_t ac = ((r & ~0x10) + 1) & 0x10;
        cpuRegs_m.f = flags_m[++r] | (cpuRegs_m.f & 0x01) | ac;
        memIo_m.write(cpuRegs_m.hl, r);
    }

        //-- dcr r --//

    void dcrR(uint8_t * r)
    {
        cpuRegs_m.clock += 2;
        uint8_t ac = ((*r & ~0x10) - 1) & 0x10;
        cpuRegs_m.f = flags_m[--(*r)] | (cpuRegs_m.f & 0x01) | ac;
    }

    void dcrA()
    {
        dcrR(&cpuRegs_m.a);
    }

    void dcrB()
    {
        dcrR(&cpuRegs_m.b);
    }

    void dcrC()
    {
        dcrR(&cpuRegs_m.c);
    }

    void dcrD()
    {
        dcrR(&cpuRegs_m.d);
    }

    void dcrE()
    {
        dcrR(&cpuRegs_m.e);
    }

    void dcrH()
    {
        dcrR(&cpuRegs_m.h);
    }

    void dcrL()
    {
        dcrR(&cpuRegs_m.l);
    }

        //-- dcr m --//

    void dcrM()
    {
        ++cpuRegs_m.clock;
        uint8_t r;
        memIo_m.read(&r, cpuRegs_m.hl);
        uint8_t ac = ((r & ~0x10) + 1) & 0x10;
        cpuRegs_m.f = flags_m[--r] | (cpuRegs_m.f & 0x01) | ac;
        memIo_m.write(cpuRegs_m.hl, r);
    }

        //-- inx rp --//

    void inxRP(uint16_t * rp)
    {
        cpuRegs_m.clock += 2;
        ++(*rp);
    }

    void inxB()
    {
        inxRP(&cpuRegs_m.bc);
    }

    void inxD()
    {
        inxRP(&cpuRegs_m.de);
    }

    void inxH()
    {
        inxRP(&cpuRegs_m.hl);
    }
    void inxSP()
    {
        inxRP(&cpuRegs_m.sp);
    }

        //-- dcx rp --//

    void dcxRP(uint16_t * rp)
    {
        cpuRegs_m.clock += 2;
        --(*rp);
    }

    void dcxB()
    {
        dcxRP(&cpuRegs_m.bc);
    }

    void dcxD()
    {
        dcxRP(&cpuRegs_m.de);
    }

    void dcxH()
    {
        dcxRP(&cpuRegs_m.hl);
    }
    void dcxSP()
    {
        dcxRP(&cpuRegs_m.sp);
    }

    //-- dad rp --//

    void dadRP(uint16_t rp)
    {
        ++cpuRegs_m.clock;
        uint32_t tmp = uint32_t(cpuRegs_m.hl) + uint32_t(rp);
        cpuRegs_m.hl = uint16_t(tmp);
        cpuRegs_m.f = (cpuRegs_m.f & ~0x01) | (uint8_t(tmp >> 16) & 0x01);
        cpuRegs_m.clock += 6;
    }

    void dadB()
    {
        dadRP(cpuRegs_m.bc);
    }

    void dadD()
    {
        dadRP(cpuRegs_m.de);
    }

    void dadH()
    {
        dadRP(cpuRegs_m.hl);
    }

    void dadSP()
    {
        dadRP(cpuRegs_m.sp);
    }

        //-- daa; cma --//

    void daa()
    {
        ++cpuRegs_m.clock;
        uint8_t ac = cpuRegs_m.f & 0x10;
        uint8_t c = cpuRegs_m.f & 0x01;
        if((cpuRegs_m.a & 0x0f) > 9 || ac) {
            ac = ((cpuRegs_m.a & ~0x10) + 0x06) & 0x10;
            uint16_t tmp = uint16_t(cpuRegs_m.a) + 0x06;
            cpuRegs_m.a = bytelo(tmp);
            c |= bytehi(tmp) & 0x01;
        }
        if((cpuRegs_m.a & 0xf0) > 0x90 || c) {
            uint16_t tmp = uint16_t(cpuRegs_m.a) + 0x60;
            cpuRegs_m.a = bytelo(tmp);
            c |= bytehi(tmp) & 0x01;
        }
        cpuRegs_m.f = flags_m[cpuRegs_m.a] | ac | c;
    }

    void cma()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.a = ~cpuRegs_m.a;
    }

        //-- rlc; rrc; ral; rar --//

    void rlc()
    {
        ++cpuRegs_m.clock;
        uint8_t c = (cpuRegs_m.a >> 7) & 0x01;
        cpuRegs_m.a = uint8_t(cpuRegs_m.a << 1) | c;
        cpuRegs_m.f = (cpuRegs_m.f & ~0x01) | c;
    }

    void rrc()
    {
        ++cpuRegs_m.clock;
        uint8_t c = cpuRegs_m.a & 0x01;
        cpuRegs_m.a = uint8_t(cpuRegs_m.a >> 1) | uint8_t(c << 7);
        cpuRegs_m.f = (cpuRegs_m.f & ~0x01) | c;
    }

    void ral()
    {
        ++cpuRegs_m.clock;
        uint8_t c = (cpuRegs_m.a >> 7) & 0x01;
        cpuRegs_m.a = uint8_t(cpuRegs_m.a << 1) | (cpuRegs_m.f & 0x01);
        cpuRegs_m.f = (cpuRegs_m.f & ~0x01) | c;
    }

    void rar()
    {
        ++cpuRegs_m.clock;
        uint8_t c = cpuRegs_m.a & 0x01;
        cpuRegs_m.a = uint8_t(cpuRegs_m.a >> 1) | uint8_t((cpuRegs_m.f & 0x01) << 7);
        cpuRegs_m.f = (cpuRegs_m.f & ~0x01) | c;
    }

        //-- stc; cmc --//

    void stc()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.f |= CpuFlag::c;
    }

    void cmc()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.f ^= CpuFlag::c;
    }

        //-- jmp addr --//

    void jmp()
    {
        ++cpuRegs_m.clock;
        memIo_m.fetch(&cpuRegs_m.pc);
    }

    //-- j* addr --//

    void jf(uint8_t flag, bool isSet = true)
    {
        cpuRegs_m.clock += 2;
        uint16_t addr;
        memIo_m.fetch(&addr);
        if(((cpuRegs_m.f & flag) != 0) == isSet) {
            cpuRegs_m.pc = addr;
        }
    }

    void jnz()
    {
        jf(CpuFlag::z, false);
    }

    void jz()
    {
        jf(CpuFlag::z);
    }

    void jnc()
    {
        jf(CpuFlag::c, false);
    }

    void jc()
    {
        jf(CpuFlag::c);
    }

    void jpo()
    {
        jf(CpuFlag::p, false);
    }

    void jpe()
    {
        jf(CpuFlag::p);
    }

    void jp()
    {
        jf(CpuFlag::s, false);
    }

    void jm()
    {
        jf(CpuFlag::s);
    }

        //-- call addr --//

    void call()
    {
        cpuRegs_m.clock += 2;
        uint16_t addr;
        memIo_m.fetch(&addr);
        memIo_m.push(cpuRegs_m.pc);
        cpuRegs_m.pc = addr;
    }

        //-- c* addr --//

    void cf(uint8_t flag, bool isSet = true)
    {
        cpuRegs_m.clock += 2;
        uint16_t addr;
        memIo_m.fetch(&addr);
        if(((cpuRegs_m.f & flag) != 0) == isSet) {
            memIo_m.push(cpuRegs_m.pc);
            cpuRegs_m.pc = addr;
        }
    }

    void cnz()
    {
        cf(CpuFlag::z, false);
    }

    void cz()
    {
        cf(CpuFlag::z);
    }

    void cnc()
    {
        cf(CpuFlag::c, false);
    }

    void cc()
    {
        cf(CpuFlag::c);
    }

    void cpo()
    {
        cf(CpuFlag::p, false);
    }

    void cpe()
    {
        cf(CpuFlag::p);
    }

    void cp()
    {
        cf(CpuFlag::s, false);
    }

    void cm()
    {
        cf(CpuFlag::s);
    }

        //-- ret --//

    void ret()
    {
        cpuRegs_m.clock += 2;
        retHookTrigger_m.fire();
        memIo_m.pop(&cpuRegs_m.pc);
    }

        //-- r* --//

    void rf(uint8_t flag, bool isSet = true)
    {
        cpuRegs_m.clock += 2;
        if(((cpuRegs_m.f & flag) != 0) == isSet) {
            retHookTrigger_m.fire();
            memIo_m.pop(&cpuRegs_m.pc);
        }
    }

    void rnz()
    {
        rf(CpuFlag::z, false);
    }

    void rz()
    {
        rf(CpuFlag::z);
    }

    void rnc()
    {
        rf(CpuFlag::c, false);
    }

    void rc()
    {
        rf(CpuFlag::c);
    }

    void rpo()
    {
        rf(CpuFlag::p, false);
    }

    void rpe()
    {
        rf(CpuFlag::p);
    }

    void rp()
    {
        rf(CpuFlag::s, false);
    }

    void rm()
    {
        rf(CpuFlag::s);
    }

        //-- pchl --//

    void pchl()
    {
        cpuRegs_m.clock += 2;
        cpuRegs_m.pc = cpuRegs_m.hl;
    }

        //-- rst * --//

    void rst(uint16_t addr)
    {
        cpuRegs_m.clock += 2;
        memIo_m.push(cpuRegs_m.pc);
        cpuRegs_m.pc = addr;
    }

    void rst0()
    {
        rst(0x0000);
    }

    void rst1()
    {
        rst(0x0008);
    }

    void rst2()
    {
        rst(0x0010);
    }

    void rst3()
    {
        rst(0x0018);
    }

    void rst4()
    {
        rst(0x0020);
    }

    void rst5()
    {
        rst(0x0028);
    }

    void rst6()
    {
        rst(0x0030);
    }

    void rst7()
    {
        rst(0x0038);
    }

        //-- ei; di; hlt; nop --//

    void ei()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.state |= CpuState::inte;
    }

    void di()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.state &= ~CpuState::inte;
    }

    void hlt()
    {
        ++cpuRegs_m.clock;
        cpuRegs_m.state |= CpuState::halt;

        cpuRegs_m.clock += 2;
        if(cpuRegs_m.clock < clocksPerFrame) {
            cpuRegs_m.clock = clocksPerFrame;
        }
    }

    void nop()
    {
        ++cpuRegs_m.clock;
    }

        //-- in/out port --//

    void in()
    {
        ++cpuRegs_m.clock;
        uint8_t port;
        memIo_m.fetch(&port);
        cpuRegs_m.clock += 3;
        (this->*inFuncs[port])();
        ++cpuRegs_m.clock;
    }

    void out()
    {
        ++cpuRegs_m.clock;
        uint8_t port;
        memIo_m.fetch(&port);
        cpuRegs_m.clock += 4;
        (this->*outFuncs[port])();
        ++cpuRegs_m.clock;
    }

        //-- port 80 --//

    void in80()
    {
        cpuRegs_m.a = ioPorts_m->port80;
    }

    void out80()
    {
        ioPorts_m->port80 = cpuRegs_m.a;
        memIo_m.init();
    }

        //-- port 81 --//

    void in81()
    {
        cpuRegs_m.a = ioPorts_m->port81[ioPorts_m->port82 & 0x0f];
    }

        //-- port 82 --//

    void in82()
    {
        cpuRegs_m.a = ioPorts_m->port82;
    }

    void out82()
    {
        ioPorts_m->port82 = cpuRegs_m.a;
        // TODO:
        //setBeepBit(ioPorts_m->port & 0x80);
        //setTapeBit(ioPorts_m->port & 0x40);
    }

        //-- port 84 --//

    void in84()
    {
        cpuRegs_m.a = ioPorts_m->port84 | 0x0f;
    }

    void out84()
    {
        ioPorts_m->port84 = cpuRegs_m.a | 0x0f;
    }

        //-- port 85 --//

    void in85()
    {
        cpuRegs_m.a = ioPorts_m->port85;
    }

    void out85()
    {
        ioPorts_m->port85 = cpuRegs_m.a;
    }

        //-- port 86 --//

    void in86()
    {
        cpuRegs_m.a = ioPorts_m->port86 | 0xce;
    }

    void out86()
    {
        ioPorts_m->port86 = cpuRegs_m.a | 0xce;
    }

        //-- port 88 --//

    void in88()
    {
        cpuRegs_m.a = ioPorts_m->port88;
    }

    void out88()
    {
        ioPorts_m->port88 = cpuRegs_m.a;
    }

        //-- port 8c --//

    void in8c()
    {
        cpuRegs_m.a = ioPorts_m->port8c & 0x3f;
    }

        //-- port 8d --//

    void in8d()
    {
        cpuRegs_m.a = ioPorts_m->port8d & 0x3f;
        // TODO:
        //if(tapeBit()) {
        //    cpuRegs_m.a |= 0x80;
        //}
    }

        //-- port 90 --//

    void in90()
    {
        cpuRegs_m.a = ioPorts_m->port90 | 0xf0;
    }

    void out90()
    {
        ioPorts_m->port90 = cpuRegs_m.a | 0xf0;
    }

        //-- port 91 --//

    void in91()
    {
        cpuRegs_m.a = ioPorts_m->port91 | 0xf1;
    }

    void out91()
    {
        ioPorts_m->port91 = cpuRegs_m.a | 0xf1;
    }

        //-- port 92 --//

    void in92()
    {
        cpuRegs_m.a = ioPorts_m->port92 | 0xf7;
    }

    void out92()
    {
        ioPorts_m->port92 = cpuRegs_m.a | 0xf7;
    }

        //-- port 93 --//

    void in93()
    {
        cpuRegs_m.a = ioPorts_m->port93 | 0xf7;
    }

    void out93()
    {
        ioPorts_m->port93 = cpuRegs_m.a | 0xf7;
    }

        //-- no port --//

    void inNop()
    {
        cpuRegs_m.a = 0xff;
    }

    void outNop()
    {
    }

    CpuRegs cpuRegs_m;

    MemIo memIo_m;
    IoPorts * ioPorts_m;

    std::array<uint8_t, 256> flags_m;

    IntHook::HookTrigger intHookTrigger_m;
    OpHook::HookTrigger  opHookTrigger_m;
    RetHook::HookTrigger retHookTrigger_m;

    using OpFunc = void (Impl::*)();
    static constexpr std::array<OpFunc, 256> opFuncs = {
        // 0x00
        &Impl::nop,     &Impl::lxiB,    &Impl::staxB,   &Impl::inxB,
        &Impl::inrB,    &Impl::dcrB,    &Impl::mviB,    &Impl::rlc,
        &Impl::nop,     &Impl::dadB,    &Impl::ldaxB,   &Impl::dcxB,
        &Impl::inrC,    &Impl::dcrC,    &Impl::mviC,    &Impl::rrc,
        // 0x10
        &Impl::nop,     &Impl::lxiD,    &Impl::staxD,   &Impl::inxD,
        &Impl::inrD,    &Impl::dcrD,    &Impl::mviD,    &Impl::ral,
        &Impl::nop,     &Impl::dadD,    &Impl::ldaxD,   &Impl::dcxD,
        &Impl::inrE,    &Impl::dcrE,    &Impl::mviE,    &Impl::rar,
        // 0x20
        &Impl::nop,     &Impl::lxiH,    &Impl::shld,    &Impl::inxH,
        &Impl::inrH,    &Impl::dcrH,    &Impl::mviH,    &Impl::daa,
        &Impl::nop,     &Impl::dadH,    &Impl::lhld,    &Impl::dcxH,
        &Impl::inrL,    &Impl::dcrL,    &Impl::mviL,    &Impl::cma,
        // 0x30
        &Impl::nop,     &Impl::lxiSP,   &Impl::sta,     &Impl::inxSP,
        &Impl::inrM,    &Impl::dcrM,    &Impl::mviM,    &Impl::stc,
        &Impl::nop,     &Impl::dadSP,   &Impl::lda,     &Impl::dcxSP,
        &Impl::inrA,    &Impl::dcrA,    &Impl::mviA,    &Impl::cmc,
        // 0x40
        &Impl::movBB,   &Impl::movBC,   &Impl::movBD,   &Impl::movBE,
        &Impl::movBH,   &Impl::movBL,   &Impl::movBM,   &Impl::movBA,
        &Impl::movCB,   &Impl::movCC,   &Impl::movCD,   &Impl::movCE,
        &Impl::movCH,   &Impl::movCL,   &Impl::movCM,   &Impl::movCA,
        // 0x50
        &Impl::movDB,   &Impl::movDC,   &Impl::movDD,   &Impl::movDE,
        &Impl::movDH,   &Impl::movDL,   &Impl::movDM,   &Impl::movDA,
        &Impl::movEB,   &Impl::movEC,   &Impl::movED,   &Impl::movEE,
        &Impl::movEH,   &Impl::movEL,   &Impl::movEM,   &Impl::movEA,
        // 0x60
        &Impl::movHB,   &Impl::movHC,   &Impl::movHD,   &Impl::movHE,
        &Impl::movHH,   &Impl::movHL,   &Impl::movHM,   &Impl::movHA,
        &Impl::movLB,   &Impl::movLC,   &Impl::movLD,   &Impl::movLE,
        &Impl::movLH,   &Impl::movLL,   &Impl::movLM,   &Impl::movLA,
        // 0x70
        &Impl::movMB,   &Impl::movMC,   &Impl::movMD,   &Impl::movME,
        &Impl::movMH,   &Impl::movML,   &Impl::hlt,     &Impl::movMA,
        &Impl::movAB,   &Impl::movAC,   &Impl::movAD,   &Impl::movAE,
        &Impl::movAH,   &Impl::movAL,   &Impl::movAM,   &Impl::movAA,
        // 0x80
        &Impl::addB,    &Impl::addC,    &Impl::addD,    &Impl::addE,
        &Impl::addH,    &Impl::addL,    &Impl::addM,    &Impl::addA,
        &Impl::adcB,    &Impl::adcC,    &Impl::adcD,    &Impl::adcE,
        &Impl::adcH,    &Impl::adcL,    &Impl::adcM,    &Impl::adcA,
        // 0x90
        &Impl::subB,    &Impl::subC,    &Impl::subD,    &Impl::subE,
        &Impl::subH,    &Impl::subL,    &Impl::subM,    &Impl::subA,
        &Impl::sbbB,    &Impl::sbbC,    &Impl::sbbD,    &Impl::sbbE,
        &Impl::sbbH,    &Impl::sbbL,    &Impl::sbbM,    &Impl::sbbA,
        // 0xa0
        &Impl::anaB,    &Impl::anaC,    &Impl::anaD,    &Impl::anaE,
        &Impl::anaH,    &Impl::anaL,    &Impl::anaM,    &Impl::anaA,
        &Impl::xraB,    &Impl::xraC,    &Impl::xraD,    &Impl::xraE,
        &Impl::xraH,    &Impl::xraL,    &Impl::xraM,    &Impl::xraA,
        // 0xb0
        &Impl::oraB,    &Impl::oraC,    &Impl::oraD,    &Impl::oraE,
        &Impl::oraH,    &Impl::oraL,    &Impl::oraM,    &Impl::oraA,
        &Impl::cmpB,    &Impl::cmpC,    &Impl::cmpD,    &Impl::cmpE,
        &Impl::cmpH,    &Impl::cmpL,    &Impl::cmpM,    &Impl::cmpA,
        // 0xc0
        &Impl::rnz,     &Impl::popB,    &Impl::jnz,     &Impl::jmp,
        &Impl::cnz,     &Impl::pushB,   &Impl::adi,     &Impl::rst0,
        &Impl::rz,      &Impl::ret,     &Impl::jz,      &Impl::jmp,
        &Impl::cz,      &Impl::call,    &Impl::aci,     &Impl::rst1,
        // 0xd0
        &Impl::rnc,     &Impl::popD,    &Impl::jnc,     &Impl::out,
        &Impl::cnc,     &Impl::pushD,   &Impl::sui,     &Impl::rst2,
        &Impl::rc,      &Impl::ret,     &Impl::jc,      &Impl::in,
        &Impl::cc,      &Impl::call,    &Impl::sbi,     &Impl::rst3,
        // 0xe0
        &Impl::rpo,     &Impl::popH,    &Impl::jpo,     &Impl::xthl,
        &Impl::cpo,     &Impl::pushH,   &Impl::ani,     &Impl::rst4,
        &Impl::rpe,     &Impl::pchl,    &Impl::jpe,     &Impl::xchg,
        &Impl::cpe,     &Impl::call,    &Impl::xri,     &Impl::rst5,
        // 0xf0
        &Impl::rp,      &Impl::popPSW,  &Impl::jp,      &Impl::di,
        &Impl::cp,      &Impl::pushPSW, &Impl::ori,     &Impl::rst6,
        &Impl::rm,      &Impl::sphl,    &Impl::jm,      &Impl::ei,
        &Impl::cm,      &Impl::call,    &Impl::cpi,     &Impl::rst7
    };

    using InFunc = void (Impl::*)();
    static constexpr std::array<InFunc, 256> inFuncs = {
        // 0x00
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x10
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x20
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x30
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x40
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x50
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x60
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x70
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0x80
        &Impl::in80,    &Impl::in81,    &Impl::in82,    &Impl::inNop,
        &Impl::in84,    &Impl::in85,    &Impl::in86,    &Impl::inNop,
        &Impl::in88,    &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::in8c,    &Impl::in8d,    &Impl::inNop,   &Impl::inNop,
        // 0x90
        &Impl::in90,    &Impl::in91,    &Impl::in92,    &Impl::in93,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xa0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xb0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xc0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xd0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xe0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        // 0xf0
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop,
        &Impl::inNop,   &Impl::inNop,   &Impl::inNop,   &Impl::inNop
    };

    using OutFunc = void (Impl::*)();
    static constexpr std::array<OutFunc, 256> outFuncs = {
        // 0x00
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x10
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x20
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x30
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x40
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x50
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x60
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x70
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x80
        &Impl::out80,   &Impl::outNop,  &Impl::out82,   &Impl::outNop,
        &Impl::out84,   &Impl::out85,   &Impl::out86,   &Impl::outNop,
        &Impl::out88,   &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0x90
        &Impl::out90,   &Impl::out91,   &Impl::out92,   &Impl::out93,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xa0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xb0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xc0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xd0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xe0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        // 0xf0
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
        &Impl::outNop,  &Impl::outNop,  &Impl::outNop,  &Impl::outNop,
    };
};

auto Cpu::create(Memory * memory) -> std::unique_ptr<Cpu>
{
    return std::make_unique<Impl>(memory);
}
