#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "bytes.h"
#include "hooks.h"

constexpr size_t clocksPerFrame = 49280; // 308 scanlines with 160 cpu clocks per each

struct CpuFlag {
    static const uint8_t s  = 0x80;
    static const uint8_t z  = 0x40;
    static const uint8_t ac = 0x10;
    static const uint8_t p  = 0x04;
    static const uint8_t c  = 0x01;
};

struct CpuState {
    static const unsigned inte = 1;
    static const unsigned halt = 2;
};

struct CpuRegs final
{
    unsigned state;
    unsigned clock;

    union {
        uint16_t psw;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t a, f;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t f, a;
#endif
        };
    };
    union {
        uint16_t bc;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t b, c;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t c, b;
#endif
        };
    };
    union {
        uint16_t de;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t d, e;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t e, d;
#endif
        };
    };
    union {
        uint16_t hl;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t h, l;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t l, h;
#endif
        };
    };
    union {
        uint16_t sp;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t sph, spl;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t spl, sph;
#endif
        };
    };
    union {
        uint16_t pc;
        struct {
#if BYTE_ORDER == BIG_ENDIAN
            uint8_t pch, pcl;
#elif BYTE_ORDER == LITTLE_ENDIAN
            uint8_t pcl, pch;
#endif
        };
    };
};

enum struct MemAccessType {
    read, write
};

using MemHook = Hook<MemAccessType /* accessType */, MemBankType /*bankType*/, uint16_t /*addr*/>;
using IntHook = Hook<>;
using OpHook  = Hook<>;

class RetHook final
{
public:
    using HookTrigger = ::HookTrigger<>;
    using HookFunc = MemFunc<void()>;

    explicit RetHook(CpuRegs * cpuRegs, HookTrigger * trigger, const HookFunc & hookFunc);
    ~RetHook();

    void setHookFunc(const HookFunc & hookFunc);

    void activate(bool isActive = true);
    auto isActive() const -> bool;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

class Cpu:
        public ITimelineSubSystem,
        public Logger
{
public:
    static auto create(Memory * memory) -> std::unique_ptr<Cpu>;

    virtual auto cpuRegs() -> CpuRegs * = 0;

    virtual void memPeek(uint8_t * data, uint16_t addr) = 0;
    virtual void memPoke(uint16_t addr, uint8_t data) = 0;

    virtual auto createMemHook(const MemHook::HookFunc & hookFunc) -> std::unique_ptr<MemHook> = 0;
    virtual auto createIntHook(const IntHook::HookFunc & hookFunc) -> std::unique_ptr<IntHook> = 0;
    virtual auto createOpHook (const OpHook::HookFunc  & hookFunc) -> std::unique_ptr<OpHook>  = 0;
    virtual auto createRetHook(const RetHook::HookFunc & hookFunc) -> std::unique_ptr<RetHook> = 0;

private:
    class Impl;
    explicit Cpu() = default;
};

#endif // CPU_H
