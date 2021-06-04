#ifndef HOOKS_H
#define HOOKS_H

#include "memfunc.h"
#include <vector>
#include <memory>

template <typename... Args>
class Hook;

template <typename... Args>
class HookTrigger final
{
public:
    using Hook = ::Hook<Args...>;
    using HookFunc = MemFunc<void(Args...)>;

    explicit HookTrigger(HookTrigger &&) = default;
    auto operator = (HookTrigger &&) -> HookTrigger & = default;

    explicit HookTrigger():
        impl(std::make_shared<Impl>())
    {}

    void fire(Args... args)
    {
        impl->fire(args...);
    }

private:
    class Impl final
    {
    public:
        struct HookImpl final
        {
            Hook * hook;
            HookFunc hookFunc;
        };

        auto addHook(Hook * hook, const HookFunc & hookFunc) -> size_t
        {
            hookImpls_m.push_back({hook, hookFunc});
            return hookImpls_m.size() - 1;
        }

        void removeHook(size_t hookPos)
        {
            hookImpls_m.erase(hookImpls_m.begin() + hookPos);
            for(size_t pos = hookPos; pos < hookImpls_m.size(); ++pos) {
                hookImpls_m[pos].hook->posChanged(pos);
            }
        }

        void setHookFunc(size_t hookPos, const HookFunc & hookFunc)
        {
            hookImpls_m[hookPos].hookFunc = hookFunc;
        }

        void fire(Args... args)
        {
            auto it = hookImpls_m.rbegin();
            while(it != hookImpls_m.rend()) {
                it->hookFunc(args...);
                ++it;
            }
        }

        std::vector<HookImpl> hookImpls_m;
    };

    std::shared_ptr<Impl> impl;

    friend Hook;
};

template <typename... Args>
class Hook final
{
public:
    using HookTrigger = ::HookTrigger<Args...>;
    using HookFunc = MemFunc<void(Args...)>;

    explicit Hook(Hook &&) = default;
    auto operator = (Hook &&) -> Hook & = default;

    explicit Hook(HookTrigger * trigger, const HookFunc & hookFunc):
        triggerImpl_m(trigger->impl),
        thisPos_m(triggerImpl_m->addHook(this, hookFunc))
    {}

    ~Hook()
    {
        triggerImpl_m->removeHook(thisPos_m);
    }

    void setHookFunc(const HookFunc & hookFunc)
    {
        triggerImpl_m->setHookFunc(thisPos_m, hookFunc);
    }

private:
    void posChanged(size_t pos)
    {
        thisPos_m = pos;
    }

    std::shared_ptr<typename HookTrigger::Impl> triggerImpl_m;
    size_t thisPos_m;

    friend HookTrigger;
};

#endif // HOOKS_H
