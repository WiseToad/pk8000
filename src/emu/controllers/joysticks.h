#ifndef JOYSTICKS_H
#define JOYSTICKS_H

#include "keyboard.h"

constexpr size_t joystickPortCount = 2;

using RetroJoystickKey = int; // emilator's joypad button

enum class TargetJoystickKey  // PK8000's joystick button
{
    keyUnknown,

    keyUp,
    keyDown,
    keyLeft,
    keyRight,
    key1,
    key2
};

template <typename KeyT>
struct PortKey final
{
    unsigned port;
    KeyT key;
};

template <typename KeyT>
inline PortKey<KeyT> portKey(unsigned port, KeyT key)
{
    return {port, key};
}

template <typename KeyT>
inline bool operator < (const PortKey<KeyT> & lhs, const PortKey<KeyT> & rhs)
{
    return lhs.port < rhs.port || (lhs.port == rhs.port && lhs.key < rhs.key);
}

using RetroJoystickMatrix = ControllerMatrix<PortKey<RetroJoystickKey>>;
using TargetJoystickMatrix = ControllerMatrix<PortKey<TargetJoystickKey>>;

using RetroJoystickHook = Hook<PortKey<RetroJoystickKey> /* key */, bool /* isPressed */, bool * /* isConsumed */>;
using TargetJoystickHook = Hook<PortKey<TargetJoystickKey> /* key */, bool /* isPressed */, bool * /* isConsumed */>;

enum class JoystickPortMode {
    disabled,
    mapper,
    joystick
};

class Joysticks:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Memory * memory, Keyboard * keyboard) -> std::unique_ptr<Joysticks>;

    virtual void setPortMode(size_t port, JoystickPortMode portMode) = 0;

    virtual auto createRetroJoystickMatrix() -> std::unique_ptr<RetroJoystickMatrix> = 0;
    virtual auto createTargetJoystickMatrix() -> std::unique_ptr<TargetJoystickMatrix> = 0;

    virtual auto createRetroJoystickHook(const RetroJoystickHook::HookFunc & hookFunc)
                                         -> std::unique_ptr<RetroJoystickHook> = 0;
    virtual auto createTargetJoystickHook(const TargetJoystickHook::HookFunc & hookFunc)
                                          -> std::unique_ptr<TargetJoystickHook> = 0;
private:
    class Impl;
    explicit Joysticks() = default;
};

#endif // JOYSTICKS_H
