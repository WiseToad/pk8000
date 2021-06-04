#include "logfilter.h"

class LogFilter::Impl
{
public:
    explicit Impl(std::unique_ptr<ILog> log, FilterFunc filterFunc):
        log_m(std::move(log)), filterFunc_m(std::move(filterFunc))
    {}

    void msg(int level, const std::string & text)
    {
        if(log_m) {
            if(filterFunc_m) {
                filterFunc_m(level, text, log_m.get());
            } else {
                log_m->msg(level, text);
            }
        }
    }

    std::unique_ptr<ILog> log_m;
    FilterFunc filterFunc_m;
};

LogFilter::LogFilter(std::unique_ptr<ILog> log, FilterFunc filterFunc):
    impl(std::make_unique<Impl>(std::move(log), std::move(filterFunc)))
{
}

LogFilter::~LogFilter()
{
}

void LogFilter::msg(int level, const std::string & text)
{
    impl->msg(level, text);
}
