#ifndef MEMORY_H
#define MEMORY_H

#include "subsystem.h"
#include "logging.h"
#include <array>

using MemBank = std::array<uint8_t, 65536>;

struct MemBanks final
{
    MemBank ram;
    MemBank rom;
    MemBank x1;
    MemBank x2;
};

enum struct MemBankType {
    ram, rom, x1, x2
};

struct IoPorts final
{
    uint8_t port80;
    std::array<uint8_t, 16> port81;
    uint8_t port82;
    uint8_t port84;
    uint8_t port85;
    uint8_t port86;
    uint8_t port88;
    uint8_t port8c;
    uint8_t port8d;
    uint8_t port90;
    uint8_t port91;
    uint8_t port92;
    uint8_t port93;
};

class Memory:
        public ISubSystem,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<Memory>;

    virtual auto memBanks() -> MemBanks * = 0;
    virtual auto ioPorts() -> IoPorts * = 0;

private:
    class Impl;
    explicit Memory() = default;
};

#endif // MEMORY_H
