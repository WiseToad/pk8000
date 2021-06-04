#include "media.h"
#include "cas.h"
#include "wav.h"

class MediaFile::Impl final
{
public:
    explicit Impl():
        fileFmt_m(FileFmt::unknown)
    {}

    void setFileName(const std::string & fileName)
    {
        fileName_m = fileName;
        fileFmt_m = FileFmt::unknown;

        if(fileName_m.empty()) {
            return;
        }

        std::unique_ptr<FileReader> file(FileReader::create());
        file->open(fileName_m);
        if(file->isValid()) {
            if(probeFile<CasFileProber>(file.get())) {
                fileFmt_m = FileFmt::cas;
                return;
            }
            if(probeFile<WavFileProber>(file.get())) {
                fileFmt_m = FileFmt::wav;
                return;
            }
        } else {
            std::string fileExt = extractExt(fileName_m);
            if(fileExt == ".cas") {
                fileFmt_m = FileFmt::cas;
                return;
            }
            if(fileExt == ".wav") {
                fileFmt_m = FileFmt::wav;
                return;
            }
        }
    }

    template <typename P>
    auto probeFile(FileReader * file) -> bool
    {
        file->recover();
        file->seek(0);

        std::unique_ptr<P> fileProber = P::create();
        return fileProber->probe(file);
    }

    auto extractExt(const std::string & fileName) -> std::string
    {
        std::string fileExt;
        size_t pos = fileName.rfind('.');
        if(pos != std::string::npos) {
            fileExt = fileName.substr(pos);
            std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(),
                           [](char ch) { return std::tolower(ch); });
        }
        return fileExt;
    }

    std::string fileName_m;
    FileFmt fileFmt_m;
};

MediaFile::MediaFile():
    impl(std::make_unique<Impl>())
{
}

MediaFile::~MediaFile()
{
}

void MediaFile::setFileName(const std::string & fileName)
{
    impl->setFileName(fileName);
}

auto MediaFile::fileName() const -> std::string
{
    return impl->fileName_m;
}

auto MediaFile::fileFmt() const -> FileFmt
{
    return impl->fileFmt_m;
}
