#ifndef FILEPROBER_H
#define FILEPROBER_H

#include "file.h"

class FileProber:
        public Interface
{
public:
    virtual auto probe(FileReader * file) -> bool = 0;
};

#endif // FILEPROBER_H
