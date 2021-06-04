#ifndef FILE_H
#define FILE_H

#include "streams.h"
#include "logging.h"

class FileReader:
        public IRandomReader,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<FileReader>;

    virtual void open(const std::string & fileName) = 0;
    virtual void close() = 0;

private:
    class Impl;
    FileReader() = default;
};

class FileWriter:
        public IRandomWriter,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<FileWriter>;

    virtual void open(const std::string & fileName, bool trunc) = 0;
    virtual void close() = 0;

private:
    class Impl;
    FileWriter() = default;
};

#endif // FILE_H
