#ifndef FILESYS_H
#define FILESYS_H

#include "logging.h"

class FileSys final:
        public Logger
{
public:
    explicit FileSys();
    ~FileSys();

    auto mkdir(std::string dirName) -> bool;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // FILESYS_H
