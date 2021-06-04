#ifndef BYTES_H
#define BYTES_H

#include <cstdint>

#ifdef _WIN32
# include <sys/param.h>
#else
# include <endian.h>
#endif

#if !defined(BYTE_ORDER) || !(BYTE_ORDER == BIG_ENDIAN || BYTE_ORDER == LITTLE_ENDIAN)
# error Endianness constants does not specified or invalid
#endif

inline auto byteswap(uint16_t data) -> uint16_t
{
    return __builtin_bswap16(data);
}

inline auto byteswap(uint32_t data) -> uint32_t
{
    return __builtin_bswap32(data);
}

inline auto bytepack(uint8_t hi, uint8_t lo) -> uint16_t
{
    return uint16_t(uint16_t(hi) << 8) | uint16_t(lo);
}

inline auto wordpack(uint16_t hi, uint16_t lo) -> uint32_t
{
    return uint32_t(uint32_t(hi) << 16) | uint32_t(lo);
}

inline auto bytepack(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0) -> uint32_t
{
    return wordpack(bytepack(b3, b2), bytepack(b1, b0));
}

inline auto bytehi(uint16_t data) -> uint8_t
{
    return uint8_t(data >> 8);
}

inline auto bytelo(uint16_t data) -> uint8_t
{
    return uint8_t(data);
}

#endif // BYTES_H
