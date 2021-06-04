#include "tracer.h"
#include "libretro.h"
#include "video.h"
#include "file.h"
#include "filesys.h"

namespace {

class TracerImpl final:
        public Logger
{
public:
    explicit TracerImpl(Timeline * timeline, Cpu * cpu):
        timeline_m(timeline), cpu_m(cpu), cpuRegs_m(cpu->cpuRegs()),
        frameHook_m(timeline->createFrameHook(memFunc(this, &TracerImpl::frameHookFunc))),
        intHook_m(cpu->createIntHook(memFunc(this, &TracerImpl::intHookFunc))),
        intRetHook_m(cpu->createRetHook(memFunc(this, &TracerImpl::intRetHookFunc))),
        opHook_m(cpu->createOpHook(memFunc(this, &TracerImpl::opHookFunc))),
        file_m(FileWriter::create()), colNum_m(0)
    {
        captureLog(file_m.get());
        start();
    }

    ~TracerImpl()
    {
        flushToFile();
    }

private:
    void start()
    {
        std::string traceDir = LibRetro::traceDir();
        if(traceDir.empty()) {
            msg(LogLevel::error, "Trace directory doesn't specified");
            deactivateByError();
            return;
        }

        FileSys fileSys;
        captureLog(&fileSys);
        if(!fileSys.mkdir(traceDir)) {
            deactivateByError();
            return;
        }

        startNewFile();
    }

    void startNewFile()
    {
        startNewLine();

        std::string traceDir = LibRetro::traceDir();
        std::string fileName = stringf("%s/trace-%d-%06d.txt", traceDir.data(),
                                       timeline_m->startTime(), timeline_m->frameNum());
        file_m->open(fileName, true);
        if(!file_m->isValid()) {
            deactivateByError();
            return;
        }
    }

    void startNewLine()
    {
        if(colNum_m > 0) {
            colNum_m = 0;
            line_m += "\n";
            flushToFile();
        }
    }

    void flushToFile()
    {
        writeToFile(line_m);
        if(file_m->isValid()) {
            line_m.clear();
        }
    }

    void writeToFile(const std::string & line)
    {
        if(!file_m->isValid()) {
            return;
        }
        file_m->write(reinterpret_cast<const uint8_t *>(line.data()), line.size());
        if(!file_m->isValid()) {
            deactivateByError();
            return;
        }
    }

    void deactivateByError()
    {
        frameHook_m = nullptr;
        intHook_m = nullptr;
        intRetHook_m = nullptr;
        opHook_m = nullptr;

        msg(LogLevel::info, "Tracer is deactivated due to errors");
    }

    void frameHookFunc()
    {
        unsigned frame = timeline_m->frameNum();
        if(frame / videoFps * videoFps == frame) {
            startNewFile();
        } else {
            startNewLine();
        }

        unsigned f = frame % videoFps;
        frame /= videoFps;
        unsigned s = frame % 60;
        frame /= 60;
        unsigned m = frame % 60;
        frame /= 60;
        unsigned h = frame;

        writeToFile(stringf("======== frame %06d %02d:%02d:%02d-%02d %s\n",
                            timeline_m->frameNum(), h, m, s, f, std::string(46, '=').data()));
    }

    void intHookFunc()
    {
        startNewLine();
        writeToFile(stringf("-------- interrupt %s\n", std::string(61, '-').data()));
        intRetHook_m->activate();
    }

    void intRetHookFunc()
    {
        startNewLine();
        writeToFile(stringf("-------- end of interrupt %s\n", std::string(54, '-').data()));
        flushToFile();
    }

    void opHookFunc()
    {
        uint8_t op;
        cpu_m->memPeek(&op, cpuRegs_m->pc);
        line_m += stringf("%04x %02x   ", cpuRegs_m->pc, op);
        if(++colNum_m >= 8) {
            startNewLine();
        }
    }

    Timeline * timeline_m;
    Cpu * cpu_m;
    CpuRegs * cpuRegs_m;

    std::unique_ptr<FrameHook> frameHook_m;
    std::unique_ptr<IntHook> intHook_m;
    std::unique_ptr<RetHook> intRetHook_m;
    std::unique_ptr<OpHook> opHook_m;

    std::unique_ptr<FileWriter> file_m;
    std::string line_m;
    int colNum_m;
};

} // namespace

class Tracer::Impl final:
        public Tracer
{
public:
    explicit Impl(Timeline * timeline, Cpu * cpu, Keyboard * keyboard):
        timeline_m(timeline), cpu_m(cpu),
        retroKeyboardHook_m(keyboard->createRetroKeyboardHook(memFunc(this, &Impl::retroKeyboardHookFunc)))
    {}

    virtual void init() override
    {
        close();
        reset();
    }

    virtual void reset() override
    {
        if(isActive()) {
            activate(false);
            activate();
        }
    }

    virtual void close() override
    {
        activate(false);
    }

    virtual void activate(bool isActive = true) override
    {
        if(isActive == this->isActive()) {
            return;
        }
        if(isActive) {
            tracer_m = std::make_unique<TracerImpl>(timeline_m, cpu_m);
            captureLog(tracer_m.get());
        } else {
            tracer_m = nullptr;
        }
    }

    virtual auto isActive() const -> bool override
    {
        return (tracer_m.get() != nullptr);
    }

private:
    void retroKeyboardHookFunc(RetroKeyboardKey key, bool isPressed, bool * isConsumed)
    {
        if(*isConsumed) {
            return;
        }
        switch(key) {
            case RETROK_F7:
                if(isPressed) {
                    activate(!isActive());
                    LibRetro::popupMsg("Trace %s", (isActive() ? "ON" : "OFF"));
                    *isConsumed = true;
                }
                break;
            default:
                break;
        }
    }

    Timeline * timeline_m;
    Cpu * cpu_m;

    std::unique_ptr<RetroKeyboardHook> retroKeyboardHook_m;

    std::unique_ptr<TracerImpl> tracer_m;
};

auto Tracer::create(Timeline * timeline, Cpu * cpu, Keyboard * keyboard) -> std::unique_ptr<Tracer>
{
    return std::make_unique<Impl>(timeline, cpu, keyboard);
}
