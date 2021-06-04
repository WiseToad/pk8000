#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include <memory>

enum struct FileFmt {
    unknown, cas, wav
};

class MediaFile final
{
public:
    explicit MediaFile();
    ~MediaFile();

    void setFileName(const std::string & fileName);
    auto fileName() const -> std::string;
    auto fileFmt() const -> FileFmt;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

struct Media final
{
    MediaFile playbackFile;
    MediaFile recordFile;
};

#endif // MEDIA_H
