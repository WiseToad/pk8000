#include "cas.h"
#include "file.h"
#include "streamer.h"
#include "slice.h"
#include <vector>
#include <cstring>

namespace {

static constexpr uint8_t casHeaderSignature[] = { 0x1f, 0xa6, 0xde, 0xba, 0xcc, 0x13, 0x7d, 0x74 };

struct CasHeader final
{
    uint8_t signature[8];

    void init()
    {
        memcpy(signature, casHeaderSignature, sizeof(casHeaderSignature));
    }

    void read(Streamer<FileReader> * streamer)
    {
        streamer->read(signature, sizeof(signature) / sizeof(signature[0]));
        if(!streamer->isValid()) {
            return;
        }
        if(memcmp(signature, casHeaderSignature, sizeof(casHeaderSignature))) {
            streamer->msg(LogLevel::error, "Invalid CAS file header");
            streamer->invalidate();
            return;
        }
    }

    void write(Streamer<FileWriter> * streamer)
    {
        streamer->write(signature, sizeof(signature) / sizeof(signature[0]));
    }
};

template <typename FileT>
class CasFileImpl:
        public Logger
{
public:
    explicit CasFileImpl():
        file_m(FileT::create()), isOpen_m(false)
    {
        captureLog(file_m.get());
    }

protected:
    std::unique_ptr<FileT> file_m;
    bool isOpen_m;
    bool isValid_m;
};

class CasFileReaderImpl final:
        public CasFileImpl<FileReader>
{
public:
    explicit CasFileReaderImpl(size_t bufSize):
        buf_m(std::max(bufSize, sizeof(casHeaderSignature)))
    {}

    void open(const std::string & fileName)
    {
        close();

        file_m->open(fileName);
        if(!file_m->isValid()) {
            return;
        }

        CasHeader casHeader;
        Streamer<FileReader> streamer(file_m.get());
        casHeader.read(&streamer);
        if(!streamer.isValid()) {
            return;
        }

        isOpen_m = true;
        isValid_m = true;

        signaturePos_m = 0;
        readFile();
    }

    void close()
    {
        file_m->close();
        isOpen_m = false;
    }

    void nextBlock()
    {
        if(!isValid()) {
            return;
        }

        while(signaturePos_m < sizeof(casHeaderSignature)) {
            if(!file_m->isValid() || file_m->isEof()) {
                isValid_m = false;
                return;
            }
            readFile();
        }
        signaturePos_m = 0;
        findBlockHeader();
    }

    auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t
    {
        if(!isValid()) {
            return 0;
        }

        Slice<uint8_t> target(buf, count);
        while(target.count() > 0 && block_m.count() > 0) {
            size_t chunkSize = std::min(target.count(), block_m.count());
            memcpy(target.data(), block_m.data(), chunkSize);
            target.advance(chunkSize);
            block_m.advance(chunkSize);
            if(block_m.count() == 0 && signaturePos_m < sizeof(casHeaderSignature)) {
                readFile();
            }
        }
        if(target.count() > 0 && exact) {
            msg(LogLevel::error, "Could not read desired amount of data");
            isValid_m = false;
        }
        return (count - target.count());
    }

    auto isEof() const -> bool
    {
        return (!isOpen_m || (block_m.count() == 0 && (
                                  signaturePos_m >= sizeof(casHeaderSignature) ||
                                  file_m->isEof())));
    }

    auto isValid() const -> bool
    {
        return (isOpen_m && isValid_m && (
                    block_m.count() > 0 ||
                    signaturePos_m >= sizeof(casHeaderSignature) ||
                    file_m->isValid()));
    }

    void recover()
    {
        file_m->recover();
        isValid_m = true;
    }

private:
    void readFile()
    {
        auto buf = Slice<uint8_t>(buf_m.data(), buf_m.size());
        avail_m = Slice<const uint8_t>(buf_m.data(), 0);

        memcpy(buf.data(), casHeaderSignature, signaturePos_m);
        buf.advance(signaturePos_m);
        avail_m.expand(signaturePos_m);

        size_t actualCount = file_m->read(buf.data(), buf.count(), false);
        avail_m.expand(actualCount);

        findBlockHeader();
    }

    void findBlockHeader()
    {
        block_m = Slice<const uint8_t>(avail_m.data(), 0);

        avail_m.advance(signaturePos_m);
        block_m.expand(signaturePos_m);
        while(avail_m.count() > 0 && signaturePos_m < sizeof(casHeaderSignature)) {
            if(casHeaderSignature[signaturePos_m] != *avail_m.data()) {
                signaturePos_m = 0;
            } else {
                ++signaturePos_m;
            }
            avail_m.advance();
            block_m.expand();
        }
        if(signaturePos_m < sizeof(casHeaderSignature) &&
                avail_m.count() == 0 && file_m->isEof())
        {
            signaturePos_m = 0;
        }
        block_m.reduce(signaturePos_m);
    }

    std::vector<uint8_t> buf_m;
    Slice<const uint8_t> avail_m;
    Slice<const uint8_t> block_m;
    size_t signaturePos_m;
};

class CasFileWriterImpl final:
        public CasFileImpl<FileWriter>
{
public:
    void open(const std::string & fileName, bool trunc)
    {
        close();

        if(!trunc) {
            std::unique_ptr<FileReader> file = FileReader::create();
            captureLog(file.get());
            file->open(fileName);
            if(!file->isValid()) {
                trunc = true;
            } else {
                CasHeader casHeader;
                Streamer<FileReader> streamer(file.get());
                casHeader.read(&streamer);
                if(!streamer.isValid()) {
                    return;
                }
            }
        }

        file_m->open(fileName, trunc);
        if(!file_m->isValid()) {
            return;
        }

        isOpen_m = true;
        isValid_m = true;

        if(trunc) {
            writeCasHeader();
        }
    }

    void close()
    {
        file_m->close();
        isOpen_m = false;
    }

    void addBlock()
    {
        if(!isValid()) {
            return;
        }
        writeCasHeader();
    }

    void write(const uint8_t * buf, size_t count)
    {
        if(!isValid()) {
            return;
        }
        file_m->write(buf, count);
    }

    void flush()
    {
        if(!isValid()) {
            return;
        }
        file_m->flush();
    }

    auto isValid() const -> bool
    {
        return (isOpen_m && isValid_m && file_m->isValid());
    }

    void recover()
    {
        file_m->recover();
        isValid_m = true;
    }

private:
    void writeCasHeader()
    {
        CasHeader casHeader;
        casHeader.init();

        Streamer<FileWriter> streamer(file_m.get());
        casHeader.write(&streamer);
        if(!streamer.isValid()) {
            isValid_m = false;
            return;
        }
    }
};

} // namespace

class CasFileReader::Impl final:
        public CasFileReader
{
public:
    explicit Impl(size_t bufSize):
        file_m(bufSize)
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

    virtual void nextBlock() override
    {
        file_m.nextBlock();
    }

    virtual auto read(uint8_t * buf, size_t count, bool exact = true) -> size_t override
    {
        return file_m.read(buf, count, exact);
    }

    virtual auto isEof() const -> bool override
    {
        return file_m.isEof();
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
    CasFileReaderImpl file_m;
};

auto CasFileReader::create(size_t bufSize) -> std::unique_ptr<CasFileReader>
{
    return std::make_unique<Impl>(bufSize);
}

class CasFileWriter::Impl final:
        public CasFileWriter
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

    virtual void addBlock() override
    {
        file_m.addBlock();
    }

    virtual void write(const uint8_t * buf, size_t count) override
    {
        file_m.write(buf, count);
    }

    virtual void flush() override
    {
        file_m.flush();
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
    CasFileWriterImpl file_m;
};

auto CasFileWriter::create() -> std::unique_ptr<CasFileWriter>
{
    return std::make_unique<Impl>();
}

class CasFileProber::Impl:
        public CasFileProber
{
public:
    virtual auto probe(FileReader * file) -> bool override
    {
        CasHeader casHeader;

        Streamer<FileReader> streamer(file);
        streamer.read(casHeader.signature, sizeof(casHeader.signature) / sizeof(casHeader.signature[0]));
        if(!streamer.isValid()) {
            return false;
        }
        if(memcmp(casHeader.signature, casHeaderSignature, sizeof(casHeaderSignature))) {
            return false;
        }
        return true;
    }
};

auto CasFileProber::create() -> std::unique_ptr<CasFileProber>
{
    return std::make_unique<Impl>();
}
