#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "controllers.h"
#include "hooks.h"
#include "memory.h"

using RetroKeyboardKey = int; // emilator's keyboard key

enum class TargetKeyboardKey  // PK8000's keyboard key
{
    keyUnknown,

    key0,           // 0 /
    key1,           // 1 / !
    key2,           // 2 / "
    key3,           // 3 / #
    key4,           // 4 / $
    key5,           // 5 / %
    key6,           // 6 / &
    key7,           // 7 / '

    key8,           // 8 / (
    key9,           // 9 / )
    keyLessThan,    // < / ,
    keyEquals,      // = / -
    keyGreaterThan, // > / .
    keyMultiply,    // * / :
    keyPlus,        // + / ;
    keySlash,       // / / ?

    keyLeftBracket, // Ш / [ / {
    keyBackslash,   // Э / \ / |
    keyRightBracket,// Щ / ] / }
    keyCaret,       // Ч / ^ / ~
    keyUnderscore,  // Ъ / _
    keyAt,          // Ю / @
    keyA,           // А / A
    keyB,           // Б / B

    keyC,           // Ц / C
    keyD,           // Д / D
    keyE,           // Е / E
    keyF,           // Ф / F
    keyG,           // Г / G
    keyH,           // Х / H
    keyI,           // И / I
    keyJ,           // Й / J

    keyK,           // К / K
    keyL,           // Л / L
    keyM,           // М / M
    keyN,           // Н / N
    keyO,           // О / O
    keyP,           // П / P
    keyQ,           // Я / Q
    keyR,           // Р / R

    keyS,           // С / S
    keyT,           // Т / T
    keyU,           // У / U
    keyV,           // Ж / V
    keyW,           // В / W
    keyX,           // Ь / X
    keyY,           // Ы / Y
    keyZ,           // З / Z

    keyRG,          // РГ
    keyUpr,         // УПР
    keyGrf,         // ГРФ
    keyAlf,         // АЛФ
    keyCircle,      // ○
    keyF1,          // F1 / F6
    keyF2,          // F2 / F7
    keyF3,          // F3 / F8

    keyF4,          // F4 / F9
    keyF5,          // F5 / F10
    keyPrf,         // ПРФ
    keyTab,         // ТАБ
    keyStop,        // СТОП
    keyBackspace,   // <=
    keySel,         // СЕЛ
    keyEnter,       // ВВОД

    keySpace,       // ПРОБЕЛ
    keyStrn,        // СТРН
    keyVz,          // ВЗ
    keyIz,          // ИЗ
    keyKp4,         // 4 / <
    keyKp8,         // 8 / ˄
    keyKp2,         // 2 / ˅
    keyKp6,         // 6 / >

    keyKp7,         // 7 / ↖
    keyKp9,         // 9 / ↘
    keyKp5,         // 5 / МЕНЮ
    keyKp1,         // 1 / ←
    keyKp3,         // 3 / →
    keyKpPeriod,    // .
    keyKp0,         // 0

    keyUprStop      // УПР-СТОП
};

using RetroKeyboardMatrix = ControllerMatrix<RetroKeyboardKey>;
using TargetKeyboardMatrix = ControllerMatrix<TargetKeyboardKey>;

using RetroKeyboardHook = Hook<RetroKeyboardKey /* key */, bool /* isPressed */, bool * /* isConsumed */>;
using TargetKeyboardHook = Hook<TargetKeyboardKey /* key */, bool /* isPressed */, bool * /* isConsumed */>;

class Keyboard:
        public ISubSystem,
        public Logger
{
public:
    static auto create(Memory * memory) -> std::unique_ptr<Keyboard>;

    virtual auto createRetroKeyboardMatrix() -> std::unique_ptr<RetroKeyboardMatrix> = 0;
    virtual auto createTargetKeyboardMatrix() -> std::unique_ptr<TargetKeyboardMatrix> = 0;

    virtual auto createRetroKeyboardHook(const RetroKeyboardHook::HookFunc & hookFunc)
                                         -> std::unique_ptr<RetroKeyboardHook> = 0;
    virtual auto createTargetKeyboardHook(const TargetKeyboardHook::HookFunc & hookFunc)
                                          -> std::unique_ptr<TargetKeyboardHook> = 0;
private:
    class Impl;
    explicit Keyboard() = default;
};

#endif
