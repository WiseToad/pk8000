#ifndef MEMFUNC_H
#define MEMFUNC_H

template <typename>
class MemFunc;

template <typename R, typename... Args>
class MemFunc<R(Args...)> final
{
public:
    template <typename T>
    using InstPtr = T *;

    template <typename T>
    using FuncPtr = R (T::*)(Args...);

    explicit MemFunc():
        inst_m(nullptr), func_m(nullptr)
    {}

    template <typename T>
    explicit MemFunc(InstPtr<T> inst, FuncPtr<T> func):
        inst_m(reinterpret_cast<InstPtr<X>>(inst)),
        func_m(reinterpret_cast<FuncPtr<X>>(func))
    {}

    explicit operator bool () const
    {
        return (inst_m && func_m);
    }

    auto operator () (Args... args) const -> R
    {
        return (inst_m && func_m ? (inst_m->*func_m)(args...) : R());
    }

private:
    class X;
    InstPtr<X> inst_m;
    FuncPtr<X> func_m;
};

template <typename T, typename R, typename... Args>
inline auto memFunc(T * inst, R (T::* func)(Args...)) -> MemFunc<R(Args...)>
{
    return MemFunc<R(Args...)>(inst, func);
}

#endif // MEMFUNC_H
