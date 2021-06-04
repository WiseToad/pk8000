#ifndef FILELOG_H
#define FILELOG_H

#include "logging.h"

class FileLog final:
        public ILog
{
public:
    explicit FileLog(const std::string & fileName);
    virtual ~FileLog() override;

    virtual void msg(int level, const std::string & msg) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // FILELOG_H
