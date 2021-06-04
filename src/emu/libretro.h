#ifndef LIBRETRO_H
#define LIBRETRO_H

#include "libretro/1.7.3/libretro.h"
#include "stringf.h"

class LibRetro final
{
public:
    static void popupMsg(const std::string & text);

    template <typename... Args>
    static void popupMsg(const std::string & text, Args&&... args)
    {
        popupMsg(stringf(text.data(), std::forward<Args>(args)...));
    }

    static auto systemDir() -> std::string;
    static auto saveDir() -> std::string;
    static auto dumpDir() -> std::string;
    static auto traceDir() -> std::string;
    static auto keylogDir() -> std::string;
};

#endif // LIBRETRO_H
