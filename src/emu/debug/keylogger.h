#ifndef KEYLOGGER_H
#define KEYLOGGER_H

#include "timeline.h"
#include "keyboard.h"
#include "joysticks.h"

class Keylogger:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Timeline * timeline, Keyboard * keyboard, Joysticks * joysticks) -> std::unique_ptr<Keylogger>;

    virtual void activate(bool isActive = true) = 0;
    virtual auto isActive() const -> bool = 0;

private:
    class Impl;
    explicit Keylogger() = default;
};

#endif // KEYLOGGER_H
