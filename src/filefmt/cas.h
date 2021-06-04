#ifndef CAS_H
#define CAS_H

#include "streams.h"
#include "logging.h"
#include "fileprober.h"

class CasFileReader:
        public ISequentalReader,
        public Logger
{
public:
    static auto create(size_t bufSize = 4096) -> std::unique_ptr<CasFileReader>;

    virtual void open(const std::string & fileName) = 0;
    virtual void close() = 0;

    virtual void nextBlock() = 0;

private:
    class Impl;
    explicit CasFileReader() = default;
};

class CasFileWriter:
        public ISequentalWriter,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<CasFileWriter>;

    virtual void open(const std::string & fileName, bool trunc) = 0;
    virtual void close() = 0;

    virtual void addBlock() = 0;

private:
    class Impl;
    explicit CasFileWriter() = default;
};

class CasFileProber:
        public FileProber
{
public:
    static auto create() -> std::unique_ptr<CasFileProber>;

private:
    class Impl;
    explicit CasFileProber() = default;
};

#endif // CAS_H
