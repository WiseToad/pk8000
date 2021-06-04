#ifndef TRACER_H
#define TRACER_H

#include "timeline.h"
#include "cpu.h"
#include "keyboard.h"

class Tracer:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Timeline * timeline, Cpu * cpu, Keyboard * keyboard) -> std::unique_ptr<Tracer>;

    virtual void activate(bool isActive = true) = 0;
    virtual auto isActive() const -> bool = 0;

private:
    class Impl;
    explicit Tracer() = default;
};

#endif // TRACER_H
