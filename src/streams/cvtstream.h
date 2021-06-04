#ifndef CVTSTREAM_H
#define CVTSTREAM_H

#include "slice.h"
#include "streams.h"
#include "logging.h"

class ICvt:
        public Interface
{
public:
    virtual void convert(Slice<const uint8_t> * source, Slice<uint8_t> * target) = 0;

    virtual auto isValid() const -> bool = 0;
    virtual void recover() = 0;
};

class CvtReader:
        public ISequentalReader,
        public Logger
{
    static auto create(ICvt * converter, size_t bufSize = 4096) -> std::unique_ptr<CvtReader>;

    virtual void open(ISequentalReader * source) = 0;
    virtual void close() = 0;

private:
    class Impl;
    CvtReader() = default;
};

class CvtWriter:
        public ISequentalWriter,
        public Logger
{
public:
    static auto create(ICvt * converter, size_t bufSize = 4096) -> std::unique_ptr<CvtWriter>;

    virtual void open(ISequentalWriter * target) = 0;
    virtual void close() = 0;

private:
    class Impl;
    CvtWriter() = default;
};

#endif // CVTSTREAM_H
