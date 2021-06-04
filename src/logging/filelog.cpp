#include "filelog.h"
#include "file.h"

class FileLog::Impl final
{
public:
    explicit Impl(const std::string & fileName):
        file_m(FileWriter::create())
    {
        file_m->open(fileName, false);
    }

    void msg(int level, const std::string & msg)
    {
        file_m->write(reinterpret_cast<const uint8_t *>(msg.data()), msg.length());
        file_m->flush();
    }

    std::unique_ptr<FileWriter> file_m;
};

FileLog::FileLog(const std::string & fileName):
    impl(std::make_unique<Impl>(fileName))
{
}

FileLog::~FileLog()
{
}

void FileLog::msg(int level, const std::string & msg)
{
    impl->msg(level, msg);
}
