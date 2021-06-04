#include "file.h"
#include <fstream>
#include <cstring>

namespace {

template <typename>
class FileStreamBase;

template <>
class FileStreamBase<std::ifstream>
{
protected:
    void seek(std::ios::off_type pos, std::ios::seekdir dir)
    {
        file_m.seekg(pos, dir);
    }

    auto pos() -> std::ios::pos_type
    {
        return file_m.tellg();
    }

    std::ifstream file_m;
};

template <>
class FileStreamBase<std::ofstream>
{
protected:
    void seek(std::ios::off_type pos, std::ios::seekdir dir)
    {
        file_m.seekp(pos, dir);
    }

    auto pos() -> std::ios::pos_type
    {
        return file_m.tellp();
    }

    std::ofstream file_m;
};

template <typename FileT>
class FileStreamImpl:
        public FileStreamBase<FileT>,
        public Logger
{
public:
    using Base = FileStreamBase<FileT>;
    using Base::file_m;

    void seek(size_t pos, SeekOrigin origin)
    {
        if(!isValid()) {
            return;
        }

        std::ios::seekdir dir;
        switch(origin) {
            case SeekOrigin::begin:
            default:
                dir = std::ios::beg;
                break;
            case SeekOrigin::current:
                dir = std::ios::cur;
                break;
            case SeekOrigin::end:
                dir = std::ios::end;
                break;
        }

        Base::seek(std::ios::off_type(pos), dir);
        if(!file_m) {
            msg(LogLevel::error, "Could not seek file: %s", strerror(errno));
        }
    }

    auto pos() -> size_t
    {
        if(!file_m.is_open() || file_m.bad()) {
            return 0;
        }
        return size_t(Base::pos());
    }

    auto isValid() const -> bool
    {
        return (file_m.is_open() && file_m);
    }

    void recover()
    {
        file_m.clear(file_m.rdstate() & ~std::ios::failbit);
    }
};

class FileReaderImpl final:
        public FileStreamImpl<std::ifstream>
{
public:
    void open(const std::string & fileName)
    {
        close();

        file_m.open(fileName, std::ios::binary | std::ios::in);
        if(!file_m) {
            msg(LogLevel::error, "Could not open file \"%s\": %s",
                fileName.data(), strerror(errno));
        }
    }

    void close()
    {
        if(file_m.is_open()) {
            file_m.close();
        }
    }

    auto read(uint8_t * buf, size_t count, bool exact) -> size_t
    {
        if(!isValid()) {
            return 0;
        }

        auto desiredCount = std::streamsize(count);
        file_m.read(reinterpret_cast<char *>(buf), desiredCount);
        std::streamsize actualCount = file_m.gcount();
        if(file_m.bad()) {
            msg(LogLevel::error, "Could not read file: %s", strerror(errno));
        } else
        if(actualCount < desiredCount) {
            if(exact) {
                msg(LogLevel::error, "Could not read desired amount of data");
            } else {
                file_m.clear(file_m.rdstate() & ~std::ios::failbit);
            }
        }
        return size_t(actualCount);
    }

    auto isEof() const -> bool
    {
        return (!file_m.is_open() || file_m.bad() || file_m.eof());
    }
};

class FileWriterImpl final:
        public FileStreamImpl<std::ofstream>
{
public:
    void open(const std::string & fileName, bool trunc)
    {
        close();

        if(!trunc) {
            file_m.open(fileName, std::ios::binary | std::ios::out | std::ios::in | std::ios::ate);
        }
        if(!file_m.is_open()) {
            file_m.open(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
        }
        if(!file_m) {
            msg(LogLevel::error, "Could not open file \"%s\": %s",
                fileName.data(), strerror(errno));
        }
    }

    void close()
    {
        if(file_m.is_open()) {
            file_m.close();
        }
    }

    void write(const uint8_t * buf, size_t count)
    {
        if(!isValid()) {
            return;
        }

        file_m.write(reinterpret_cast<const char *>(buf), std::streamsize(count));
        if(!file_m) {
            msg(LogLevel::error, "Could not write file: %s", strerror(errno));
        }
    }

    void flush()
    {
        if(!isValid()) {
            return;
        }

        file_m.flush();
        if(!file_m) {
            msg(LogLevel::error, "Could not flush file: %s", strerror(errno));
        }
    }
};

} // namespace

class FileReader::Impl final:
        public FileReader
{
public:
    explicit Impl()
    {
        captureLog(&file_m);
    }

    virtual void open(const std::string & fileName) override
    {
        file_m.open(fileName);
    }

    virtual void close() override
    {
        file_m.close();
    }

    virtual auto read(uint8_t * buf, size_t count, bool exact) -> size_t override
    {
        return file_m.read(buf, count, exact);
    }

    virtual auto isEof() const -> bool override
    {
        return file_m.isEof();
    }

    virtual void seek(size_t pos, SeekOrigin origin) override
    {
        file_m.seek(pos, origin);
    }

    virtual auto pos() -> size_t override
    {
        return file_m.pos();
    }

    virtual auto isValid() const -> bool override
    {
        return file_m.isValid();
    }

    virtual void recover() override
    {
        file_m.recover();
    }

private:
    FileReaderImpl file_m;
};

auto FileReader::create() -> std::unique_ptr<FileReader>
{
    return std::make_unique<Impl>();
}

class FileWriter::Impl final:
        public FileWriter
{
public:
    explicit Impl()
    {
        captureLog(&file_m);
    }

    virtual void open(const std::string & fileName, bool trunc) override
    {
        file_m.open(fileName, trunc);
    }

    virtual void close() override
    {
        file_m.close();
    }

    virtual void write(const uint8_t * buf, size_t count) override
    {
        file_m.write(buf, count);
    }

    virtual void flush() override
    {
        file_m.flush();
    }

    virtual void seek(size_t pos, SeekOrigin origin) override
    {
        file_m.seek(pos, origin);
    }

    virtual auto pos() -> size_t override
    {
        return file_m.pos();
    }

    virtual auto isValid() const -> bool override
    {
        return file_m.isValid();
    }

    virtual void recover() override
    {
        file_m.recover();
    }

private:
    FileWriterImpl file_m;
};

auto FileWriter::create() -> std::unique_ptr<FileWriter>
{
    return std::make_unique<Impl>();
}
