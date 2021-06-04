#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H

#include "interface.h"

class ISubSystem:
        public Interface
{
public:
    virtual void init() = 0;
    virtual void reset() = 0;
    virtual void close() = 0;
};

class ITimelineSubSystem:
        public ISubSystem
{
public:
    virtual void startFrame() = 0;
    virtual void renderFrame() = 0;
    virtual void endFrame() = 0;
};

#endif // SUBSYSTEM_H
