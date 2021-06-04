#include "keyboard.h"
#include "machine.h"
#include "libretro.h"
#include <map>

namespace {

using TargetKeyboardMap = std::map<RetroKeyboardKey, TargetKeyboardKey>;

using PortMask = std::array<uint8_t, 16>;
using PortMap = std::map<TargetKeyboardKey, PortMask>;

const TargetKeyboardMap targetKeyboardMap =
{
    // Windows mappings

    std::make_pair(RETROK_0,                            TargetKeyboardKey::key0),
    std::make_pair(RETROK_1,                            TargetKeyboardKey::key1),
    std::make_pair(RETROK_2,                            TargetKeyboardKey::key2),
    std::make_pair(RETROK_3,                            TargetKeyboardKey::key3),
    std::make_pair(RETROK_4,                            TargetKeyboardKey::key4),
    std::make_pair(RETROK_5,                            TargetKeyboardKey::key5),
    std::make_pair(RETROK_6,                            TargetKeyboardKey::key6),
    std::make_pair(RETROK_7,                            TargetKeyboardKey::key7),

    std::make_pair(RETROK_8,                            TargetKeyboardKey::key8),
    std::make_pair(RETROK_9,                            TargetKeyboardKey::key9),
    std::make_pair(RETROK_COMMA,                        TargetKeyboardKey::keyLessThan),
    std::make_pair(RETROK_MINUS,                        TargetKeyboardKey::keyEquals),
    std::make_pair(RETROK_PERIOD,                       TargetKeyboardKey::keyGreaterThan),
    std::make_pair(RETROK_SEMICOLON,                    TargetKeyboardKey::keyMultiply),
    std::make_pair(RETROK_EQUALS,                       TargetKeyboardKey::keyPlus),
    std::make_pair(RETROK_SLASH,                        TargetKeyboardKey::keySlash),

    std::make_pair(RETROK_LEFTBRACKET,                  TargetKeyboardKey::keyLeftBracket),
    std::make_pair(RETROK_BACKSLASH,                    TargetKeyboardKey::keyBackslash),
    std::make_pair(RETROK_RIGHTBRACKET,                 TargetKeyboardKey::keyRightBracket),
    std::make_pair(RETROK_BACKQUOTE,                    TargetKeyboardKey::keyCaret),
    std::make_pair(RETROK_QUOTE,                        TargetKeyboardKey::keyUnderscore),
    // no suitable PC key left for TargetKey::keyAt
    std::make_pair(RETROK_a,                            TargetKeyboardKey::keyA),
    std::make_pair(RETROK_b,                            TargetKeyboardKey::keyB),

    std::make_pair(RETROK_c,                            TargetKeyboardKey::keyC),
    std::make_pair(RETROK_d,                            TargetKeyboardKey::keyD),
    std::make_pair(RETROK_e,                            TargetKeyboardKey::keyE),
    std::make_pair(RETROK_f,                            TargetKeyboardKey::keyF),
    std::make_pair(RETROK_g,                            TargetKeyboardKey::keyG),
    std::make_pair(RETROK_h,                            TargetKeyboardKey::keyH),
    std::make_pair(RETROK_i,                            TargetKeyboardKey::keyI),
    std::make_pair(RETROK_j,                            TargetKeyboardKey::keyJ),

    std::make_pair(RETROK_k,                            TargetKeyboardKey::keyK),
    std::make_pair(RETROK_l,                            TargetKeyboardKey::keyL),
    std::make_pair(RETROK_m,                            TargetKeyboardKey::keyM),
    std::make_pair(RETROK_n,                            TargetKeyboardKey::keyN),
    std::make_pair(RETROK_o,                            TargetKeyboardKey::keyO),
    std::make_pair(RETROK_p,                            TargetKeyboardKey::keyP),
    std::make_pair(RETROK_q,                            TargetKeyboardKey::keyQ),
    std::make_pair(RETROK_r,                            TargetKeyboardKey::keyR),

    std::make_pair(RETROK_s,                            TargetKeyboardKey::keyS),
    std::make_pair(RETROK_t,                            TargetKeyboardKey::keyT),
    std::make_pair(RETROK_u,                            TargetKeyboardKey::keyU),
    std::make_pair(RETROK_v,                            TargetKeyboardKey::keyV),
    std::make_pair(RETROK_w,                            TargetKeyboardKey::keyW),
    std::make_pair(RETROK_x,                            TargetKeyboardKey::keyX),
    std::make_pair(RETROK_y,                            TargetKeyboardKey::keyY),
    std::make_pair(RETROK_z,                            TargetKeyboardKey::keyZ),

    std::make_pair(RETROK_RSHIFT,                       TargetKeyboardKey::keyRG),
    std::make_pair(RETROK_LSHIFT,                       TargetKeyboardKey::keyRG/*2*/), // second mapping to the same target key
    // no suitable PC key left for TargetKey::keyUpr, but it used later in compound TargetKey::keyUprStop
    std::make_pair(RETROK_RALT,                         TargetKeyboardKey::keyGrf),
    std::make_pair(RETROK_LALT,                         TargetKeyboardKey::keyAlf),
    std::make_pair(RETROK_LCTRL,                        TargetKeyboardKey::keyCircle),
    std::make_pair(RETROK_F1,                           TargetKeyboardKey::keyF1),
    std::make_pair(RETROK_F2,                           TargetKeyboardKey::keyF2),
    std::make_pair(RETROK_F3,                           TargetKeyboardKey::keyF3),

    std::make_pair(RETROK_F4,                           TargetKeyboardKey::keyF4),
    std::make_pair(RETROK_F5,                           TargetKeyboardKey::keyF5),
    std::make_pair(RETROK_ESCAPE,                       TargetKeyboardKey::keyPrf),
    std::make_pair(RETROK_TAB,                          TargetKeyboardKey::keyTab),
    std::make_pair(RETROK_F11,                          TargetKeyboardKey::keyStop),
    std::make_pair(RETROK_BACKSPACE,                    TargetKeyboardKey::keyBackspace),
    std::make_pair(RETROK_RCTRL,                        TargetKeyboardKey::keySel),     // RetroArch maps it as RETROK_LCTRL, so this PC key isn't available
    std::make_pair(RETROK_RETURN,                       TargetKeyboardKey::keyEnter),   // RetroArch maps here  RETROK_KP_ENTER too

    std::make_pair(RETROK_SPACE,                        TargetKeyboardKey::keySpace),
    std::make_pair(RETROK_KP_MULTIPLY,                  TargetKeyboardKey::keyStrn),
    std::make_pair(RETROK_KP_PLUS,                      TargetKeyboardKey::keyVz),
    std::make_pair(RETROK_INSERT,                       TargetKeyboardKey::keyVz/*2*/), // RetroArch maps it as RETROK_KP0, so this PC key isn't available
    std::make_pair(RETROK_KP_MINUS,                     TargetKeyboardKey::keyIz),
    std::make_pair(RETROK_DELETE,                       TargetKeyboardKey::keyIz/*2*/), // RetroArch maps it as RETROK_KP_PERIOD, so this PC key isn't available
    std::make_pair(RETROK_KP4,                          TargetKeyboardKey::keyKp4),     // RetroArch maps here  RETROK_LEFT too
    std::make_pair(RETROK_KP8,                          TargetKeyboardKey::keyKp8),     // RetroArch maps here  RETROK_UP too
    std::make_pair(RETROK_KP2,                          TargetKeyboardKey::keyKp2),     // RetroArch maps here  RETROK_DOWN too
    std::make_pair(RETROK_KP6,                          TargetKeyboardKey::keyKp6),     // RetroArch maps here  RETROK_RIGHT too

    std::make_pair(RETROK_KP7,                          TargetKeyboardKey::keyKp7),
    std::make_pair(RETROK_KP9,                          TargetKeyboardKey::keyKp9),
    std::make_pair(RETROK_KP5,                          TargetKeyboardKey::keyKp5),
    std::make_pair(RETROK_KP1,                          TargetKeyboardKey::keyKp1),
    std::make_pair(RETROK_KP3,                          TargetKeyboardKey::keyKp3),
    std::make_pair(RETROK_KP_PERIOD,                    TargetKeyboardKey::keyKpPeriod),
    std::make_pair(RETROK_KP0,                          TargetKeyboardKey::keyKp0),
    // no 8th mapping in this block

    std::make_pair(RETROK_F12,                          TargetKeyboardKey::keyUprStop),

    // Additional Linux mappings

    std::make_pair(RETROK_RIGHTPAREN,                   TargetKeyboardKey::key0),
    std::make_pair(RETROK_EXCLAIM,                      TargetKeyboardKey::key1),
    std::make_pair(RETROK_AT,                           TargetKeyboardKey::key2),
    std::make_pair(RETROK_HASH,                         TargetKeyboardKey::key3),
    std::make_pair(RETROK_DOLLAR,                       TargetKeyboardKey::key4),
    std::make_pair(RETROK_AMPERSAND,                    TargetKeyboardKey::key7),
    std::make_pair(RETROK_ASTERISK,                     TargetKeyboardKey::key8),
    std::make_pair(RETROK_LEFTPAREN,                    TargetKeyboardKey::key9),

    std::make_pair(RETROK_UNDERSCORE,                   TargetKeyboardKey::keyEquals),
    std::make_pair(RETROK_PLUS,                         TargetKeyboardKey::keyPlus),
    std::make_pair(RETROK_COLON,                        TargetKeyboardKey::keyMultiply),
    std::make_pair(RETROK_QUOTEDBL,                     TargetKeyboardKey::keyUnderscore),
    std::make_pair(RETROK_LESS,                         TargetKeyboardKey::keyLessThan),
    std::make_pair(RETROK_GREATER,                      TargetKeyboardKey::keyGreaterThan),
    std::make_pair(RETROK_QUESTION,                     TargetKeyboardKey::keySlash)

    // TODO
};

const PortMap portMap =
{                                                       // considering last six elements will be value-initialized, i.e. with 0
    std::make_pair(TargetKeyboardKey::key0,             PortMask{0x01,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key1,             PortMask{0x02,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key2,             PortMask{0x04,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key3,             PortMask{0x08,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key4,             PortMask{0x10,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key5,             PortMask{0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key6,             PortMask{0x40,    0,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key7,             PortMask{0x80,    0,    0,    0,    0,    0,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::key8,             PortMask{   0, 0x01,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::key9,             PortMask{   0, 0x02,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyLessThan,      PortMask{   0, 0x04,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyEquals,        PortMask{   0, 0x08,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyGreaterThan,   PortMask{   0, 0x10,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyMultiply,      PortMask{   0, 0x20,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyPlus,          PortMask{   0, 0x40,    0,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keySlash,         PortMask{   0, 0x80,    0,    0,    0,    0,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyLeftBracket,   PortMask{   0,    0, 0x01,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyBackslash,     PortMask{   0,    0, 0x02,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyRightBracket,  PortMask{   0,    0, 0x04,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyCaret,         PortMask{   0,    0, 0x08,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyUnderscore,    PortMask{   0,    0, 0x10,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyAt,            PortMask{   0,    0, 0x20,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyA,             PortMask{   0,    0, 0x40,    0,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyB,             PortMask{   0,    0, 0x80,    0,    0,    0,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyC,             PortMask{   0,    0,    0, 0x01,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyD,             PortMask{   0,    0,    0, 0x02,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyE,             PortMask{   0,    0,    0, 0x04,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyF,             PortMask{   0,    0,    0, 0x08,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyG,             PortMask{   0,    0,    0, 0x10,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyH,             PortMask{   0,    0,    0, 0x20,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyI,             PortMask{   0,    0,    0, 0x40,    0,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyJ,             PortMask{   0,    0,    0, 0x80,    0,    0,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyK,             PortMask{   0,    0,    0,    0, 0x01,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyL,             PortMask{   0,    0,    0,    0, 0x02,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyM,             PortMask{   0,    0,    0,    0, 0x04,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyN,             PortMask{   0,    0,    0,    0, 0x08,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyO,             PortMask{   0,    0,    0,    0, 0x10,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyP,             PortMask{   0,    0,    0,    0, 0x20,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyQ,             PortMask{   0,    0,    0,    0, 0x40,    0,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyR,             PortMask{   0,    0,    0,    0, 0x80,    0,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyS,             PortMask{   0,    0,    0,    0,    0, 0x01,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyT,             PortMask{   0,    0,    0,    0,    0, 0x02,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyU,             PortMask{   0,    0,    0,    0,    0, 0x04,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyV,             PortMask{   0,    0,    0,    0,    0, 0x08,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyW,             PortMask{   0,    0,    0,    0,    0, 0x10,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyX,             PortMask{   0,    0,    0,    0,    0, 0x20,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyY,             PortMask{   0,    0,    0,    0,    0, 0x40,    0,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyZ,             PortMask{   0,    0,    0,    0,    0, 0x80,    0,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyRG,            PortMask{   0,    0,    0,    0,    0,    0, 0x01,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyUpr,           PortMask{   0,    0,    0,    0,    0,    0, 0x02,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyGrf,           PortMask{   0,    0,    0,    0,    0,    0, 0x04,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyAlf,           PortMask{   0,    0,    0,    0,    0,    0, 0x08,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyCircle,        PortMask{   0,    0,    0,    0,    0,    0, 0x10,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyF1,            PortMask{   0,    0,    0,    0,    0,    0, 0x20,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyF2,            PortMask{   0,    0,    0,    0,    0,    0, 0x40,    0,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyF3,            PortMask{   0,    0,    0,    0,    0,    0, 0x80,    0,    0,    0}),

    std::make_pair(TargetKeyboardKey::keyF4,            PortMask{   0,    0,    0,    0,    0,    0,    0, 0x01,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyF5,            PortMask{   0,    0,    0,    0,    0,    0,    0, 0x02,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyPrf,           PortMask{   0,    0,    0,    0,    0,    0,    0, 0x04,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyTab,           PortMask{   0,    0,    0,    0,    0,    0,    0, 0x08,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyStop,          PortMask{   0,    0,    0,    0,    0,    0,    0, 0x10,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyBackspace,     PortMask{   0,    0,    0,    0,    0,    0,    0, 0x20,    0,    0}),
    std::make_pair(TargetKeyboardKey::keySel,           PortMask{   0,    0,    0,    0,    0,    0,    0, 0x40,    0,    0}),
    std::make_pair(TargetKeyboardKey::keyEnter,         PortMask{   0,    0,    0,    0,    0,    0,    0, 0x80,    0,    0}),

    std::make_pair(TargetKeyboardKey::keySpace,         PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x01,    0}),
    std::make_pair(TargetKeyboardKey::keyStrn,          PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x02,    0}),
    std::make_pair(TargetKeyboardKey::keyVz,            PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x04,    0}),
    std::make_pair(TargetKeyboardKey::keyIz,            PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x08,    0}),
    std::make_pair(TargetKeyboardKey::keyKp4,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x10,    0}),
    std::make_pair(TargetKeyboardKey::keyKp8,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x20,    0}),
    std::make_pair(TargetKeyboardKey::keyKp2,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x40,    0}),
    std::make_pair(TargetKeyboardKey::keyKp6,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0, 0x80,    0}),

    std::make_pair(TargetKeyboardKey::keyKp7,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x01}),
    std::make_pair(TargetKeyboardKey::keyKp9,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x02}),
    std::make_pair(TargetKeyboardKey::keyKp5,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x04}),
    std::make_pair(TargetKeyboardKey::keyKp1,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x08}),
    std::make_pair(TargetKeyboardKey::keyKp3,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x10}),
    std::make_pair(TargetKeyboardKey::keyKpPeriod,      PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x20}),
    std::make_pair(TargetKeyboardKey::keyKp0,           PortMask{   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x40}),
    // no 8th mapping in this block

    std::make_pair(TargetKeyboardKey::keyUprStop,       PortMask{   0,    0,    0,    0,    0,    0, 0x02, 0x10,    0,    0})
};

} // namespace

class Keyboard::Impl:
        public Keyboard
{
public:
    explicit Impl(Memory * memory):
        ioPorts_m(memory->ioPorts()),
        targetKeyboardMatrix_m(&targetKeyboard_m)
    {
        retroKeyboard_m.setStateFunc(memFunc(this, &Impl::retroKeyboardStateFunc));
        targetKeyboard_m.setStateFunc(memFunc(this, &Impl::targetKeyboardStateFunc));
    }

    virtual void init() override
    {
        reset();
    }

    virtual void reset() override
    {
        retroKeyboard_m.reset();
        targetKeyboard_m.reset();
    }

    virtual void close() override
    {
    }

    virtual auto createRetroKeyboardMatrix() -> std::unique_ptr<RetroKeyboardMatrix> override
    {
        return std::make_unique<RetroKeyboardMatrix>(&retroKeyboard_m);
    }

    virtual auto createTargetKeyboardMatrix() -> std::unique_ptr<TargetKeyboardMatrix> override
    {
        return std::make_unique<TargetKeyboardMatrix>(&targetKeyboard_m);
    }

    virtual auto createRetroKeyboardHook(const RetroKeyboardHook::HookFunc & hookFunc)
                                         -> std::unique_ptr<RetroKeyboardHook> override
    {
        return std::make_unique<RetroKeyboardHook>(&retroKeyboardHookTrigger_m, hookFunc);
    }

    virtual auto createTargetKeyboardHook(const TargetKeyboardHook::HookFunc & hookFunc)
                                          -> std::unique_ptr<TargetKeyboardHook> override
    {
        return std::make_unique<TargetKeyboardHook>(&targetKeyboardHookTrigger_m, hookFunc);
    }

private:
    void retroKeyboardStateFunc(RetroKeyboardKey key, bool isPressed)
    {
        bool isConsumed = false;
        retroKeyboardHookTrigger_m.fire(key, isPressed, &isConsumed);
        if(isConsumed) {
            return;
        }

        auto it = targetKeyboardMap.find(key);
        if(it != targetKeyboardMap.end()) {
            targetKeyboardMatrix_m.setPressed(it->second, isPressed);
        }
    }

    void targetKeyboardStateFunc(TargetKeyboardKey key, bool isPressed)
    {
        bool isConsumed = false;
        targetKeyboardHookTrigger_m.fire(key, isPressed, &isConsumed);
        if(isConsumed) {
            return;
        }

        auto it = portMap.find(key);
        if(it != portMap.end()) {
            const PortMask & portMask = it->second;
            if(isPressed) {
                for(size_t i = 0; i < ioPorts_m->port81.size(); ++i) {
                    ioPorts_m->port81[i] &= ~portMask[i];
                }
            } else {
                for(size_t i = 0; i < ioPorts_m->port81.size(); ++i) {
                    ioPorts_m->port81[i] |= portMask[i];
                }
            }
        }
    }

    IoPorts * ioPorts_m;

    Controller<RetroKeyboardKey> retroKeyboard_m;
    Controller<TargetKeyboardKey> targetKeyboard_m;
    ControllerMatrix<TargetKeyboardKey> targetKeyboardMatrix_m;

    RetroKeyboardHook::HookTrigger retroKeyboardHookTrigger_m;
    TargetKeyboardHook::HookTrigger targetKeyboardHookTrigger_m;
};

auto Keyboard::create(Memory * memory) -> std::unique_ptr<Keyboard>
{
    return std::make_unique<Impl>(memory);
}
