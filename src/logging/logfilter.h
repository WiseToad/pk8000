#ifndef LOGFILTER_H
#define LOGFILTER_H

#include "logging.h"
#include <functional>

class LogFilter final:
        public ILog
{
public:
    using FilterFunc = std::function<void(int, const std::string &, ILog * log)>;

    explicit LogFilter(std::unique_ptr<ILog> log, FilterFunc filterFunc);
    virtual ~LogFilter() override;

    virtual void msg(int level, const std::string & text) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // LOGFILTER_H
