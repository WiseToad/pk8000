#ifndef TIMELINE_H
#define TIMELINE_H

#include "subsystem.h"
#include "hooks.h"
#include "logging.h"
#include <ctime>

using FrameHook = Hook<>;

class Timeline:
        public ITimelineSubSystem,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<Timeline>;

    virtual time_t startTime() = 0;
    virtual unsigned frameNum() = 0;

    virtual auto createFrameHook(const FrameHook::HookFunc & hookFunc) -> std::unique_ptr<FrameHook> = 0;
private:
    class Impl;
    explicit Timeline() = default;
};

#endif // TIMELINE_H
