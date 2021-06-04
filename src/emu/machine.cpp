#include "machine.h"

class Machine::Impl final:
        public Machine
{
public:
    explicit Impl():
        timeline_m(Timeline::create()),
        memory_m(Memory::create()),
        cpu_m(Cpu::create(memory_m.get())),
        bios_m(Bios::create(&media_m, memory_m.get(), cpu_m.get())),
        video_m(Video::create(memory_m.get())),
        keyboard_m(Keyboard::create(memory_m.get())),
        joysticks_m(Joysticks::create(memory_m.get(), keyboard_m.get())),
        dumper_m(Dumper::create(timeline_m.get(), memory_m.get(), cpu_m.get(), keyboard_m.get())),
        tracer_m(Tracer::create(timeline_m.get(), cpu_m.get(), keyboard_m.get())),
        keylogger_m(Keylogger::create(timeline_m.get(), keyboard_m.get(), joysticks_m.get()))
    {
        captureLog(timeline_m.get());
        captureLog(memory_m.get());
        captureLog(cpu_m.get());
        captureLog(bios_m.get());
        captureLog(video_m.get());
        captureLog(keyboard_m.get());
        captureLog(joysticks_m.get());
        captureLog(dumper_m.get());
        captureLog(tracer_m.get());
        captureLog(keylogger_m.get());
    }

    virtual void init() override
    {
        timeline_m->init();
        memory_m->init();
        cpu_m->init();
        bios_m->init();
        video_m->init();
        keyboard_m->init();
        joysticks_m->init();
        dumper_m->init();
        tracer_m->init();
        keylogger_m->init();
    }

    virtual void reset() override
    {
        timeline_m->reset();
        memory_m->reset();
        cpu_m->reset();
        bios_m->reset();
        video_m->reset();
        keyboard_m->reset();
        joysticks_m->reset();
        dumper_m->reset();
        tracer_m->reset();
        keylogger_m->reset();
    }

    virtual void close() override
    {
        keylogger_m->close();
        tracer_m->close();
        dumper_m->close();
        joysticks_m->close();
        keyboard_m->close();
        video_m->close();
        bios_m->close();
        cpu_m->close();
        memory_m->close();
        timeline_m->close();
    }

    virtual void startFrame() override
    {
        timeline_m->startFrame();
        cpu_m->startFrame();
        video_m->startFrame();
    }

    virtual void renderFrame() override
    {
        timeline_m->renderFrame();
        cpu_m->renderFrame();
        video_m->renderFrame();
    }

    virtual void endFrame() override
    {
        video_m->endFrame();
        cpu_m->endFrame();
        timeline_m->endFrame();
    }

    virtual auto media() -> Media * override
    {
        return &media_m;
    }

    virtual auto timeline() -> Timeline * override
    {
        return timeline_m.get();
    }

    virtual auto memory() -> Memory * override
    {
        return memory_m.get();
    }

    virtual auto cpu() -> Cpu * override
    {
        return cpu_m.get();
    }

    virtual auto bios() -> Bios * override
    {
        return bios_m.get();
    }

    virtual auto video() -> Video * override
    {
        return video_m.get();
    }

    virtual auto keyboard() -> Keyboard * override
    {
        return keyboard_m.get();
    }

    virtual auto joysticks() -> Joysticks * override
    {
        return joysticks_m.get();
    }

    virtual auto dumper() -> Dumper * override
    {
        return dumper_m.get();
    }

    virtual auto tracer() -> Tracer * override
    {
        return tracer_m.get();
    }

    virtual auto keylogger() -> Keylogger * override
    {
        return keylogger_m.get();
    }

private:
    Media media_m;

    std::unique_ptr<Timeline> timeline_m;
    std::unique_ptr<Memory> memory_m;
    std::unique_ptr<Cpu> cpu_m;
    std::unique_ptr<Bios> bios_m;
    std::unique_ptr<Video> video_m;
    std::unique_ptr<Keyboard> keyboard_m;
    std::unique_ptr<Joysticks> joysticks_m;

    std::unique_ptr<Dumper> dumper_m;
    std::unique_ptr<Tracer> tracer_m;
    std::unique_ptr<Keylogger> keylogger_m;
};

auto Machine::create() -> std::unique_ptr<Machine>
{
    return std::make_unique<Impl>();
}
