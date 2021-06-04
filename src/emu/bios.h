#ifndef BIOS_H
#define BIOS_H

#include "media.h"
#include "memory.h"
#include "cpu.h"

class Bios:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Media * media, Memory * memory, Cpu * cpu) -> std::unique_ptr<Bios>;

    virtual void initMediaHooks() = 0;

private:
    class Impl;
    explicit Bios() = default;
};

#endif // BIOS_H
