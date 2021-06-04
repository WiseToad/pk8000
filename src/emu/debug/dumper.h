#ifndef DUMPER_H
#define DUMPER_H

#include "timeline.h"
#include "memory.h"
#include "cpu.h"
#include "keyboard.h"

class Dumper:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Timeline * timeline, Memory * memory, Cpu * cpu, Keyboard * keyboard)
                       -> std::unique_ptr<Dumper>;

    virtual void dump() = 0;

private:
    class Impl;
    explicit Dumper() = default;
};

#endif // DUMPER_H
