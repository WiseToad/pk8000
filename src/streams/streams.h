#ifndef STREAMS_H
#define STREAMS_H

#include "interface.h"
#include <stdint.h>
#include <stddef.h>

class IStream:
        public Interface
{
public:
    virtual auto isValid() const -> bool = 0;
    virtual void recover() = 0;
};

enum struct SeekOrigin {
    begin, current, end
};

class ISeekableStream:
        public Interface
{
public:
    virtual void seek(size_t pos, SeekOrigin origin = SeekOrigin::begin) = 0;
    virtual auto pos() -> size_t = 0;
};

class ISequentalReader:
        public IStream
{
public:
    virtual auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t = 0;
    virtual auto isEof() const -> bool = 0;
};

class ISequentalWriter:
        public IStream
{
public:
    virtual void write(const uint8_t * buf, size_t count) = 0;
    virtual void flush() = 0;
};

class IRandomReader:
        public ISequentalReader, public ISeekableStream
{
};

class IRandomWriter:
        public ISequentalWriter, public ISeekableStream
{
};

#endif // STREAMS_H
