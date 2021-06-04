#include "timeline.h"

class Timeline::Impl:
        public Timeline
{
public:
    virtual void init() override
    {
        reset();
    }

    virtual void reset() override
    {
        startTime_m = time(nullptr);
        frameNum_m = 0;
    }

    virtual void close() override
    {
    }

    virtual void startFrame() override
    {
    }

    virtual void renderFrame() override
    {
        frameHookTrigger_m.fire();
    }

    virtual void endFrame() override
    {
        ++frameNum_m;
    }

    virtual time_t startTime() override
    {
        return startTime_m;
    }

    virtual unsigned frameNum() override
    {
        return frameNum_m;
    }

    virtual auto createFrameHook(const FrameHook::HookFunc & hookFunc) -> std::unique_ptr<FrameHook> override
    {
        return std::make_unique<FrameHook>(&frameHookTrigger_m, hookFunc);
    }

private:
    time_t startTime_m;
    unsigned frameNum_m;

    FrameHook::HookTrigger frameHookTrigger_m;
};

auto Timeline::create() -> std::unique_ptr<Timeline>
{
    return std::make_unique<Impl>();
}
