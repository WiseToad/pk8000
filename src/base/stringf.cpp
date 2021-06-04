#include "stringf.h"
#include <cstdarg>
#include <cstring>

auto stringf(const char * fmt, ...) -> std::string
{
    constexpr size_t bufSize = 4096;

    char buf[bufSize];

    va_list args;
    va_start(args, fmt);

    int n = vsnprintf(buf, bufSize, fmt, args);
    if(size_t(n) >= bufSize - 1) {
        strcat(buf + bufSize - 4, "...");
    }

    va_end(args);

    return buf;
}
