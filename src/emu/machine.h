#ifndef MACHINE_H
#define MACHINE_H

#include "media.h"
#include "timeline.h"
#include "memory.h"
#include "cpu.h"
#include "bios.h"
#include "video.h"
#include "keyboard.h"
#include "joysticks.h"
#include "dumper.h"
#include "tracer.h"
#include "keylogger.h"

class Machine:
        public ITimelineSubSystem,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<Machine>;

    virtual auto media() -> Media * = 0;

    virtual auto timeline() -> Timeline * = 0;
    virtual auto memory() -> Memory * = 0;
    virtual auto cpu() -> Cpu * = 0;
    virtual auto bios() -> Bios * = 0;
    virtual auto video() -> Video * = 0;
    virtual auto keyboard() -> Keyboard * = 0;
    virtual auto joysticks() -> Joysticks * = 0;

    virtual auto dumper() -> Dumper * = 0;
    virtual auto tracer() -> Tracer * = 0;
    virtual auto keylogger() -> Keylogger * = 0;

private:
    class Impl;
    explicit Machine() = default;
};

#endif // MACHINE_H
