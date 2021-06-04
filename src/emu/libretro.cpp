#include "libretro.h"
#include "machine.h"
#include <cstring>

#define RETRO_DEVICE_MAPPER RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

RETRO_CALLCONV void retro_keyboard_event(bool is_pressed, unsigned key, uint32_t key_char, uint16_t key_mod);

namespace {

class LibRetroLog final:
        public ILog
{
public:
    virtual void msg(int level, const std::string & text) override;
};

struct LibRetroCallbacks final
{
    retro_environment_t environment = nullptr;
    retro_log_printf_t log_printf = nullptr;
    retro_video_refresh_t video_refresh = nullptr;
    retro_audio_sample_batch_t audio_sample_batch = nullptr;
    retro_input_poll_t input_poll = nullptr;
    retro_input_state_t input_state = nullptr;
};

class LibRetroImpl final:
        public Logger
{
public:
    static auto instance() -> LibRetroImpl *
    {
        static LibRetroImpl instance;
        return &instance;
    }

    void retro_get_system_info(retro_system_info * system_info)
    {
        system_info->library_name     = "PK8000";
        system_info->library_version  = "1.0";
        system_info->valid_extensions = "cas|rom|bin|wav|pmi|pcs|ptp|pwe";
        system_info->need_fullpath    = true;
        system_info->block_extract    = true;
    }

    void retro_set_environment(retro_environment_t environment)
    {
        callbacks_m.environment = environment;

        static constexpr bool support_no_game = true;
        callbacks_m.environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME,
                                const_cast<bool *>(&support_no_game));
    }

    void retro_init()
    {
        retro_log_callback log_callback;
        if(callbacks_m.environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_callback)) {
            callbacks_m.log_printf = log_callback.log;
            AppLog::setLog(std::make_unique<LibRetroLog>());
        } else {
            callbacks_m.log_printf = nullptr;
        }

        const char * system_dir = nullptr;
        if(callbacks_m.environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) &&
                system_dir != nullptr && system_dir[0] != 0)
        {
            systemDir_m = std::string(system_dir) + "/pk8000";
        }

        const char * save_dir = nullptr;
        if(callbacks_m.environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) &&
                save_dir != nullptr && save_dir[0] != 0)
        {
            saveDir_m = std::string(save_dir) + "/pk8000";
            dumpDir_m = saveDir_m + "/dump";
            traceDir_m = saveDir_m + "/trace";
            keylogDir_m = saveDir_m + "/keylog";
        }

        if(saveDir_m.empty() && !systemDir_m.empty()) {
            saveDir_m = systemDir_m + "/save";
        }
        if(dumpDir_m.empty() && !systemDir_m.empty()) {
            dumpDir_m = systemDir_m + "/dump";
        }
        if(traceDir_m.empty() && !systemDir_m.empty()) {
            traceDir_m = systemDir_m + "/trace";
        }
        if(keylogDir_m.empty() && !systemDir_m.empty()) {
            keylogDir_m = systemDir_m + "/keylog";
        }

        if(!systemDir_m.empty()) {
            msg(LogLevel::info, "System dir: %s", systemDir_m.data());
        }
        if(!saveDir_m.empty()) {
            msg(LogLevel::info, "Save dir: %s", saveDir_m.data());
        }
        if(!dumpDir_m.empty()) {
            msg(LogLevel::info, "Dump dir: %s", dumpDir_m.data());
        }
        if(!traceDir_m.empty()) {
            msg(LogLevel::info, "Trace dir: %s", traceDir_m.data());
        }
        if(!keylogDir_m.empty()) {
            msg(LogLevel::info, "Keylog dir: %s", keylogDir_m.data());
        }

        static constexpr retro_controller_description controller_description[] = {
            {"Joystick",        RETRO_DEVICE_JOYPAD},
            {"Keyboard Mapper", RETRO_DEVICE_MAPPER},
            {}
        };
        static constexpr retro_controller_info controller_info[] = {
            {controller_description, 2},
            {controller_description, 2},
            {}
        };
        callbacks_m.environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO,
                                const_cast<retro_controller_info *>(controller_info));

        machine_m->init();
    }

    void retro_reset()
    {
        machine_m->reset();
    }

    void retro_deinit()
    {
        machine_m->close();
    }

    auto retro_load_game(const retro_game_info * game_info) -> bool
    {
        PixelFormat pixelFormat = PixelFormat::xrgb8888;
        int pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
        if(!callbacks_m.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format)) {
            pixelFormat = PixelFormat::rgb565;
            pixel_format = RETRO_PIXEL_FORMAT_RGB565;
            if(!callbacks_m.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format)) {
                msg(LogLevel::error, "Could not set pixel format");
                return false;
            }
        }
        machine_m->video()->setPixelFormat(pixelFormat);

        static constexpr retro_keyboard_callback keyboard_callback = {
            &::retro_keyboard_event
        };
        if(!callbacks_m.environment(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK,
                                    const_cast<retro_keyboard_callback *>(&keyboard_callback)))
        {
            msg(LogLevel::warn, "Could not set keyboard callback");
        }

        machine_m->media()->playbackFile.setFileName(game_info != nullptr ? game_info->path : "");
        machine_m->media()->recordFile.setFileName(!saveDir_m.empty() ? saveDir_m + "/record.wav" : "");
        machine_m->bios()->initMediaHooks();

        return true;
    }

    auto retro_load_game_special(unsigned game_type, const retro_game_info * game_info, size_t num_info) -> bool
    {
        return false;
    }

    void retro_unload_game()
    {
    }

    unsigned retro_api_version()
    {
        return RETRO_API_VERSION;
    }

    unsigned retro_get_region()
    {
        return RETRO_REGION_PAL;
    }

    void retro_set_video_refresh(retro_video_refresh_t video_refresh)
    {
        callbacks_m.video_refresh = video_refresh;
    }

    void retro_set_audio_sample(retro_audio_sample_t audio_sample)
    {
    }

    void retro_set_audio_sample_batch(retro_audio_sample_batch_t audio_sample_batch)
    {
        callbacks_m.audio_sample_batch = audio_sample_batch;
    }

    void retro_set_input_state(retro_input_state_t input_state)
    {
        callbacks_m.input_state = input_state;
    }

    void retro_set_input_poll(retro_input_poll_t input_poll)
    {
        callbacks_m.input_poll = input_poll;
    }

    void retro_get_system_av_info(retro_system_av_info * av_info)
    {
        av_info->geometry.base_width = videoWidth;
        av_info->geometry.base_height = videoHeight;
        av_info->geometry.max_width = videoWidth;
        av_info->geometry.max_height = videoHeight;
        av_info->geometry.aspect_ratio = 0.0;

        av_info->timing.fps = double(videoFps);
        // TODO:
        //av_info->timing.sample_rate = double(audioSampleRate);
    }

    void retro_run()
    {
        machine_m->startFrame();

        callbacks_m.input_poll();
        for(unsigned port = 0; port < joystickPortCount; ++port) {
            for(int key: {RETRO_DEVICE_ID_JOYPAD_B,
                          RETRO_DEVICE_ID_JOYPAD_Y,
                          RETRO_DEVICE_ID_JOYPAD_SELECT,
                          RETRO_DEVICE_ID_JOYPAD_START,
                          RETRO_DEVICE_ID_JOYPAD_UP,
                          RETRO_DEVICE_ID_JOYPAD_DOWN,
                          RETRO_DEVICE_ID_JOYPAD_LEFT,
                          RETRO_DEVICE_ID_JOYPAD_RIGHT,
                          RETRO_DEVICE_ID_JOYPAD_A,
                          RETRO_DEVICE_ID_JOYPAD_X,
                          RETRO_DEVICE_ID_JOYPAD_L,
                          RETRO_DEVICE_ID_JOYPAD_R,
                          RETRO_DEVICE_ID_JOYPAD_L2,
                          RETRO_DEVICE_ID_JOYPAD_R2,
                          RETRO_DEVICE_ID_JOYPAD_L3,
                          RETRO_DEVICE_ID_JOYPAD_R3})
            {
                bool isPressed = callbacks_m.input_state(port, RETRO_DEVICE_JOYPAD, 0, unsigned(key));
                retroJoystickMatrix_m->setPressed(portKey(port, key), isPressed);
            }
        }

        machine_m->renderFrame();
        machine_m->endFrame();

        FrameBuffer * frameBuffer = machine_m->video()->frameBuffer();
        callbacks_m.video_refresh(frameBuffer->data(),
                                  frameBuffer->width(),
                                  frameBuffer->height(),
                                  frameBuffer->pitch());

        // TODO:
        //callbacks_m.audio_sample_batch(audioBuf.data(), samplesPerFrame);
    }

    void retro_set_controller_port_device(unsigned port, unsigned device)
    {
        switch(device) {
            case RETRO_DEVICE_JOYPAD:
                machine_m->joysticks()->setPortMode(port, JoystickPortMode::joystick);
                break;
            case RETRO_DEVICE_MAPPER:
                machine_m->joysticks()->setPortMode(port, JoystickPortMode::mapper);
                break;
            default:
                machine_m->joysticks()->setPortMode(port, JoystickPortMode::disabled);
                break;
        }
    }

    void retro_keyboard_event(bool is_pressed, unsigned key, uint32_t key_char, uint16_t key_mod)
    {
        retroKeyboardMatrix_m->setPressed(int(key), is_pressed);
    }

    auto retro_serialize_size() -> size_t
    {
        return 0;
    }

    auto retro_serialize(void * data, size_t size) -> bool
    {
        return false;
    }

    auto retro_unserialize(const void * data, size_t size) -> bool
    {
        return false;
    }

    void retro_cheat_reset()
    {
    }

    void retro_cheat_set(unsigned index, bool enabled, const char * code)
    {
    }

    void * retro_get_memory_data(unsigned id)
    {
        return nullptr;
    }

    auto retro_get_memory_size(unsigned id) -> size_t
    {
        return 0;
    }

    void logMsg(int level, const std::string & text)
    {
        if(callbacks_m.log_printf == nullptr) {
            static StdLog stdLog;
            stdLog.msg(level, text);
            return;
        }

        retro_log_level log_level;
        switch(level) {
            case LogLevel::debug:
                log_level = RETRO_LOG_DEBUG;
                break;
            case LogLevel::info:
            default:
                log_level = RETRO_LOG_INFO;
                break;
            case LogLevel::warn:
                log_level = RETRO_LOG_WARN;
                break;
            case LogLevel::error:
            case LogLevel::fatal:
                log_level = RETRO_LOG_ERROR;
                break;
        }
        callbacks_m.log_printf(log_level, "%s\n", text.data());
    }

    void popupMsg(const std::string & text)
    {
        if(callbacks_m.environment != nullptr) {
            retro_message message { text.data(), 150 };
            callbacks_m.environment(RETRO_ENVIRONMENT_SET_MESSAGE, &message);
        }
    }

    LibRetroCallbacks callbacks_m;

    std::string systemDir_m;
    std::string saveDir_m;
    std::string dumpDir_m;
    std::string traceDir_m;
    std::string keylogDir_m;

    std::unique_ptr<Machine> machine_m;
    std::unique_ptr<RetroKeyboardMatrix> retroKeyboardMatrix_m;
    std::unique_ptr<RetroJoystickMatrix> retroJoystickMatrix_m;

private:
    explicit LibRetroImpl():
        machine_m(Machine::create()),
        retroKeyboardMatrix_m(machine_m->keyboard()->createRetroKeyboardMatrix()),
        retroJoystickMatrix_m(machine_m->joysticks()->createRetroJoystickMatrix())
    {}
};

void LibRetroLog::msg(int level, const std::string & text)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->logMsg(level, text);
}

} // namespace

void LibRetro::popupMsg(const std::string & text)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->popupMsg(text);
}

auto LibRetro::systemDir() -> std::string
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->systemDir_m;
}

auto LibRetro::saveDir() -> std::string
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->saveDir_m;
}

auto LibRetro::dumpDir() -> std::string
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->dumpDir_m;
}

auto LibRetro::traceDir() -> std::string
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->traceDir_m;
}

auto LibRetro::keylogDir() -> std::string
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->keylogDir_m;
}

RETRO_API void retro_get_system_info(retro_system_info * system_info)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_get_system_info(system_info);
}

RETRO_API void retro_set_environment(retro_environment_t environment)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_environment(environment);
}

RETRO_API void retro_init()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_init();
}

RETRO_API void retro_reset()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_reset();
}

RETRO_API void retro_deinit()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_deinit();
}

RETRO_API auto retro_load_game(const retro_game_info * game_info) -> bool
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_load_game(game_info);
}

RETRO_API auto retro_load_game_special(unsigned game_type, const retro_game_info * game_info, size_t num_info) -> bool
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_load_game_special(game_type, game_info, num_info);
}

RETRO_API void retro_unload_game()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_unload_game();
}

RETRO_API auto retro_api_version() -> unsigned
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_api_version();
}

RETRO_API auto retro_get_region(void) -> unsigned
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_get_region();
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t video_refresh)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_video_refresh(video_refresh);
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t audio_sample)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_audio_sample(audio_sample);
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t audio_sample_batch)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_audio_sample_batch(audio_sample_batch);
}

RETRO_API void retro_set_input_state(retro_input_state_t input_state)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_input_state(input_state);
}

RETRO_API void retro_set_input_poll(retro_input_poll_t input_poll)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_input_poll(input_poll);
}

RETRO_API void retro_get_system_av_info(retro_system_av_info * av_info)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_get_system_av_info(av_info);
}

RETRO_API void retro_run()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_run();
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_set_controller_port_device(port, device);
}

RETRO_CALLCONV void retro_keyboard_event(bool is_pressed, unsigned key, uint32_t key_char, uint16_t key_mod)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    impl->retro_keyboard_event(is_pressed, key, key_char, key_mod);
}

RETRO_API auto retro_serialize_size() -> size_t
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_serialize_size();
}

RETRO_API auto retro_serialize(void * data, size_t size) -> bool
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_serialize(data, size);
}

RETRO_API auto retro_unserialize(const void * data, size_t size) -> bool
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_unserialize(data, size);
}

RETRO_API void retro_cheat_reset()
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_cheat_reset();
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char * code)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_cheat_set(index, enabled, code);
}

RETRO_API void * retro_get_memory_data(unsigned id)
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_get_memory_data(id);
}

RETRO_API auto retro_get_memory_size(unsigned id) -> size_t
{
    LibRetroImpl * impl = LibRetroImpl::instance();
    return impl->retro_get_memory_size(id);
}
