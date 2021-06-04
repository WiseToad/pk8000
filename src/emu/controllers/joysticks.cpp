#include "joysticks.h"
#include "machine.h"
#include "libretro.h"
#include <map>

namespace {

using TargetKeyboardMap = std::map<PortKey<RetroJoystickKey>, TargetKeyboardKey>;
using TargetJoystickMap = std::map<PortKey<RetroJoystickKey>, PortKey<TargetJoystickKey>>;

struct PortMask final
{
    unsigned port;
    uint8_t mask;
};

using PortMap = std::map<PortKey<TargetJoystickKey>, PortMask>;

const TargetKeyboardMap targetKeyboardMap =
{
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_UP),    TargetKeyboardKey::keyKp8),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_DOWN),  TargetKeyboardKey::keyKp2),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_LEFT),  TargetKeyboardKey::keyKp4),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_RIGHT), TargetKeyboardKey::keyKp6),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_A),     TargetKeyboardKey::keySpace),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_B),     TargetKeyboardKey::keyEnter),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_X),     TargetKeyboardKey::keyF1),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_Y),     TargetKeyboardKey::keyF2),

    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_UP),    TargetKeyboardKey::keyKp8),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_DOWN),  TargetKeyboardKey::keyKp2),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_LEFT),  TargetKeyboardKey::keyKp4),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_RIGHT), TargetKeyboardKey::keyKp6),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_A),     TargetKeyboardKey::keySpace),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_B),     TargetKeyboardKey::keyEnter),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_X),     TargetKeyboardKey::keyF1),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_Y),     TargetKeyboardKey::keyF2)
};

const TargetJoystickMap targetJoystickMap =
{
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_UP),    portKey(1, TargetJoystickKey::keyUp)),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_DOWN),  portKey(1, TargetJoystickKey::keyDown)),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_LEFT),  portKey(1, TargetJoystickKey::keyLeft)),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_RIGHT), portKey(1, TargetJoystickKey::keyRight)),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_A),     portKey(1, TargetJoystickKey::key1)),
    std::make_pair(portKey(1, RETRO_DEVICE_ID_JOYPAD_B),     portKey(1, TargetJoystickKey::key2)),

    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_UP),    portKey(2, TargetJoystickKey::keyUp)),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_DOWN),  portKey(2, TargetJoystickKey::keyDown)),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_LEFT),  portKey(2, TargetJoystickKey::keyLeft)),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_RIGHT), portKey(2, TargetJoystickKey::keyRight)),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_A),     portKey(2, TargetJoystickKey::key1)),
    std::make_pair(portKey(2, RETRO_DEVICE_ID_JOYPAD_B),     portKey(2, TargetJoystickKey::key2))
};

const PortMap portMap =
{
    std::make_pair(portKey(1, TargetJoystickKey::keyUp),     PortMask{1, 0x01}),
    std::make_pair(portKey(1, TargetJoystickKey::keyDown),   PortMask{1, 0x02}),
    std::make_pair(portKey(1, TargetJoystickKey::keyLeft),   PortMask{1, 0x04}),
    std::make_pair(portKey(1, TargetJoystickKey::keyRight),  PortMask{1, 0x08}),
    std::make_pair(portKey(1, TargetJoystickKey::key1),      PortMask{1, 0x10}),
    std::make_pair(portKey(1, TargetJoystickKey::key2),      PortMask{1, 0x20}),

    std::make_pair(portKey(2, TargetJoystickKey::keyUp),     PortMask{2, 0x01}),
    std::make_pair(portKey(2, TargetJoystickKey::keyDown),   PortMask{2, 0x02}),
    std::make_pair(portKey(2, TargetJoystickKey::keyLeft),   PortMask{2, 0x04}),
    std::make_pair(portKey(2, TargetJoystickKey::keyRight),  PortMask{2, 0x08}),
    std::make_pair(portKey(2, TargetJoystickKey::key1),      PortMask{2, 0x10}),
    std::make_pair(portKey(2, TargetJoystickKey::key2),      PortMask{2, 0x20})
};

} // namespace

class Joysticks::Impl final:
        public Joysticks
{
public:
    explicit Impl(Memory * memory, Keyboard * keyboard):
        ioPorts_m(memory->ioPorts()), targetJoystickMatrix_m(&targetJoystick_m),
        targetKeyboardMatrix_m(keyboard->createTargetKeyboardMatrix())
    {
        portMode_m.fill(JoystickPortMode::disabled);

        retroJoystick_m.setStateFunc(memFunc(this, &Impl::retroJoystickStateFunc));
        targetJoystick_m.setStateFunc(memFunc(this, &Impl::targetJoystickStateFunc));
    }

    virtual void init() override
    {
        reset();
    }

    virtual void reset() override
    {
        retroJoystick_m.reset();
        targetJoystick_m.reset();
        targetKeyboardMatrix_m.reset();
    }

    virtual void close() override
    {
    }

    virtual void setPortMode(size_t port, JoystickPortMode portMode) override
    {
        if(port < portMode_m.size()) {
            portMode_m[port] = portMode;
        }

        // TODO: reset affected matrices
    }

    virtual auto createRetroJoystickMatrix() -> std::unique_ptr<RetroJoystickMatrix> override
    {
        return std::make_unique<RetroJoystickMatrix>(&retroJoystick_m);
    }

    virtual auto createTargetJoystickMatrix() -> std::unique_ptr<TargetJoystickMatrix> override
    {
        return std::make_unique<TargetJoystickMatrix>(&targetJoystick_m);
    }

    virtual auto createRetroJoystickHook(const RetroJoystickHook::HookFunc & hookFunc)
                                         -> std::unique_ptr<RetroJoystickHook> override
    {
        return std::make_unique<RetroJoystickHook>(&retroJoystickHookTrigger_m, hookFunc);
    }

    virtual auto createTargetJoystickHook(const TargetJoystickHook::HookFunc & hookFunc)
                                          -> std::unique_ptr<TargetJoystickHook> override
    {
        return std::make_unique<TargetJoystickHook>(&targetJoystickHookTrigger_m, hookFunc);
    }

private:
    void retroJoystickStateFunc(PortKey<RetroJoystickKey> key, bool isPressed)
    {
        if(key.port >= portMode_m.size()) {
            return;
        }

        bool isConsumed = false;
        retroJoystickHookTrigger_m.fire(key, isPressed, &isConsumed);
        if(isConsumed) {
            return;
        }

        if(portMode_m[key.port] == JoystickPortMode::mapper) {
            auto it = targetKeyboardMap.find(key);
            if(it != targetKeyboardMap.end()) {
                targetKeyboardMatrix_m->setPressed(it->second, isPressed);
            }
        } else
        if(portMode_m[key.port] == JoystickPortMode::joystick) {
            auto it = targetJoystickMap.find(key);
            if(it != targetJoystickMap.end()) {
                targetJoystickMatrix_m.setPressed(it->second, isPressed);
            }
        }
    }

    void targetJoystickStateFunc(PortKey<TargetJoystickKey> key, bool isPressed)
    {
        bool isConsumed = false;
        targetJoystickHookTrigger_m.fire(key, isPressed, &isConsumed);
        if(isConsumed) {
            return;
        }

        auto it = portMap.find(key);
        if(it != portMap.end()) {
            std::array<uint8_t *, joystickPortCount> joystickPorts = {
                &ioPorts_m->port8c,
                &ioPorts_m->port8d
            };

            const PortMask & portMask = it->second;
            uint8_t * port = joystickPorts[portMask.port];
            uint8_t mask = portMask.mask;

            if(isPressed) {
                *port |= mask;
            } else {
                *port &= ~mask;
            }
        }
    }

    IoPorts * ioPorts_m;

    std::array<JoystickPortMode, joystickPortCount> portMode_m;

    Controller<PortKey<RetroJoystickKey>> retroJoystick_m;
    Controller<PortKey<TargetJoystickKey>> targetJoystick_m;
    ControllerMatrix<PortKey<TargetJoystickKey>> targetJoystickMatrix_m;

    std::unique_ptr<TargetKeyboardMatrix> targetKeyboardMatrix_m;

    RetroJoystickHook::HookTrigger retroJoystickHookTrigger_m;
    TargetJoystickHook::HookTrigger targetJoystickHookTrigger_m;
};

auto Joysticks::create(Memory * memory, Keyboard * keyboard) -> std::unique_ptr<Joysticks>
{
    return std::make_unique<Impl>(memory, keyboard);
}
