#ifndef REVERSE_H
#define REVERSE_H

#include "streams.h"
#include "logging.h"

class ReverseReader:
        public ISequentalReader,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<ReverseReader>;

    virtual void open(IRandomReader * source) = 0;
    virtual void close() = 0;

    virtual void setReverse(bool isReverse) = 0;
    virtual auto isReverse() const -> bool = 0;

private:
    class Impl;
    ReverseReader() = default;
};

#endif // REVERSE_H
