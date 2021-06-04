#include "logging.h"
#include <cstdio>

namespace {

class AppLogImpl final
{
public:
    static auto instance() -> AppLogImpl *
    {
        static AppLogImpl instance;
        return &instance;
    }

    void msg(int level, const std::string & text)
    {
        if(log_m) {
            log_m->msg(level, text);
        }
    }

    std::unique_ptr<ILog> log_m;

private:
    explicit AppLogImpl():
        log_m(std::make_unique<StdLog>())
    {}
};

} // namespace

void AppLog::setLog(std::unique_ptr<ILog> log)
{
    auto impl = AppLogImpl::instance();
    impl->log_m = std::move(log);
}

void AppLog::msg(int level, const std::string & text)
{
    auto impl = AppLogImpl::instance();
    impl->msg(level, text);
}

void StdLog::msg(int level, const std::string & text)
{
    fprintf(stderr, "%s\n", text.data());
}

void NullLog::msg(int level, const std::string & text)
{
}

class Logger::Impl final:
        public ILog
{
public:
    virtual void msg(int level, const std::string & text) override
    {
        if(log_m) {
            log_m->msg(level, text);
        }
    }

    std::shared_ptr<ILog> log_m;
};

Logger::Logger():
    impl(std::make_shared<Impl>())
{
    setLog(std::make_unique<AppLog>());
}

Logger::~Logger()
{
}

void Logger::setLog(std::unique_ptr<ILog> log)
{
    impl->log_m = std::move(log);
}

void Logger::captureLog(Logger * source)
{
    source->impl->log_m = impl;
}

void Logger::msg(int level, const std::string & text)
{
    impl->msg(level, text);
}
