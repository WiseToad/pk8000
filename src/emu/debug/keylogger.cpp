#include "keylogger.h"
#include "libretro.h"
#include "file.h"
#include "filesys.h"

namespace {

class KeyloggerImpl final:
        public Logger
{
public:
    explicit KeyloggerImpl(Timeline * timeline, Keyboard * keyboard, Joysticks * joysticks):
        timeline_m(timeline),
        retroKeyboardHook_m(keyboard->createRetroKeyboardHook(memFunc(this, &KeyloggerImpl::retroKeyboardHookFunc))),
        targetKeyboardHook_m(keyboard->createTargetKeyboardHook(memFunc(this, &KeyloggerImpl::targetKeyboardHookFunc))),
        retroJoystickHook_m(joysticks->createRetroJoystickHook(memFunc(this, &KeyloggerImpl::retroJoystickHookFunc))),
        targetJoystickHook_m(joysticks->createTargetJoystickHook(memFunc(this, &KeyloggerImpl::targetJoystickHookFunc))),
        file_m(FileWriter::create())
    {
        captureLog(file_m.get());
        start();
    }

private:
    void start()
    {
        std::string keylogDir = LibRetro::keylogDir();
        if(keylogDir.empty()) {
            msg(LogLevel::error, "Keylog directory doesn't specified");
            deactivateByError();
            return;
        }

        FileSys fileSys;
        captureLog(&fileSys);
        if(!fileSys.mkdir(keylogDir)) {
            deactivateByError();
            return;
        }

        std::string fileName = stringf("%s/keylog-%d.txt", keylogDir.data(), timeline_m->startTime());
        file_m->open(fileName, false);
        if(!file_m->isValid()) {
            deactivateByError();
            return;
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
        file_m->close();
        msg(LogLevel::info, "Keylogger is deactivated due to errors");
    }

    void retroKeyboardHookFunc(RetroKeyboardKey key, bool isPressed, bool * isConsumed)
    {
        writeToFile(stringf("%06d  RETRO  KEYBOARD: key %03d %s\n",
                            timeline_m->frameNum(), int(key), (isPressed ? "pressed" : "released")));
    }

    void targetKeyboardHookFunc(TargetKeyboardKey key, bool isPressed, bool * isConsumed)
    {
        writeToFile(stringf("%06d  TARGET KEYBOARD: key %03d %s\n",
                            timeline_m->frameNum(), int(key), (isPressed ? "pressed" : "released")));
    }

    void retroJoystickHookFunc(PortKey<RetroJoystickKey> key, bool isPressed, bool * isConsumed)
    {
        writeToFile(stringf("%06d  RETRO  JOYSTICK: port %d key %02d %s\n",
                            timeline_m->frameNum(), key.port, int(key.key), (isPressed ? "pressed" : "released")));
    }

    void targetJoystickHookFunc(PortKey<TargetJoystickKey> key, bool isPressed, bool * isConsumed)
    {
        writeToFile(stringf("%06d  TARGET JOYSTICK: port %d key %02d %s\n",
                            timeline_m->frameNum(), key.port, int(key.key), (isPressed ? "pressed" : "released")));
    }

    Timeline * timeline_m;

    std::unique_ptr<RetroKeyboardHook> retroKeyboardHook_m;
    std::unique_ptr<TargetKeyboardHook> targetKeyboardHook_m;
    std::unique_ptr<RetroJoystickHook> retroJoystickHook_m;
    std::unique_ptr<TargetJoystickHook> targetJoystickHook_m;

    std::unique_ptr<FileWriter> file_m;
};

} // namespace

class Keylogger::Impl final:
        public Keylogger
{
public:
    explicit Impl(Timeline * timeline, Keyboard * keyboard, Joysticks * joysticks):
        timeline_m(timeline), keyboard_m(keyboard), joysticks_m(joysticks),
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
            keylogger_m = std::make_unique<KeyloggerImpl>(timeline_m, keyboard_m, joysticks_m);
            captureLog(keylogger_m.get());
        } else {
            keylogger_m = nullptr;
        }
    }

    virtual auto isActive() const -> bool override
    {
        return (keylogger_m.get() != nullptr);
    }

private:
    void retroKeyboardHookFunc(RetroKeyboardKey key, bool isPressed, bool * isConsumed)
    {
        if(*isConsumed) {
            return;
        }
        switch(key) {
            case RETROK_F8:
                if(isPressed) {
                    activate(!isActive());
                    LibRetro::popupMsg("Keylog %s", (isActive() ? "ON" : "OFF"));
                    *isConsumed = true;
                }
                break;
            default:
                break;
        }
    }

    Timeline * timeline_m;
    Keyboard * keyboard_m;
    Joysticks * joysticks_m;

    std::unique_ptr<RetroKeyboardHook> retroKeyboardHook_m;

    std::unique_ptr<KeyloggerImpl> keylogger_m;
};

auto Keylogger::create(Timeline * timeline, Keyboard * keyboard, Joysticks * joysticks) -> std::unique_ptr<Keylogger>
{
    return std::make_unique<Impl>(timeline, keyboard, joysticks);
}
