#ifndef LOGGING_H
#define LOGGING_H

#include "interface.h"
#include "stringf.h"
#include <memory>

enum LogLevel {
    fatal, error, warn, info, debug, user
};

class ILog:
        public Interface
{
public:
    virtual void msg(int level, const std::string & text) = 0;
};

class AppLog final:
        public ILog
{
public:
    static void setLog(std::unique_ptr<ILog> log);

    virtual void msg(int level, const std::string & text) override;
};

class StdLog final:
        public ILog
{
public:
    virtual void msg(int level, const std::string & text) override;
};

class NullLog final:
        public ILog
{
public:
    virtual void msg(int level, const std::string & text) override;
};

class Logger
{
public:
    explicit Logger();
    virtual ~Logger();

    void setLog(std::unique_ptr<ILog> log);
    void captureLog(Logger * source);

    void msg(int level, const std::string & text);

    template <typename... Args>
    void msg(int level, const std::string & text, Args&&... args)
    {
        msg(level, stringf(text.data(), std::forward<Args>(args)...));
    }

private:
    class Impl;
    std::shared_ptr<Impl> impl;
};

#endif // LOGGING_H
