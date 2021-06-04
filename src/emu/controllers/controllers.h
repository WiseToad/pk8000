#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "memfunc.h"
#include <set>
#include <list>
#include <memory>

template <typename KeyT>
class ControllerMatrix;

template <typename KeyT>
class Controller final
{
public:
    using ControllerMatrix = ::ControllerMatrix<KeyT>;
    using StateFunc = MemFunc<void(KeyT /* key */, bool /* pressed */)>;

    explicit Controller(Controller &&) = default;
    auto operator = (Controller &&) -> Controller & = default;

    explicit Controller():
        impl(std::make_shared<Impl>())
    {}

    void reset()
    {
        impl->reset();
    }

    auto isPressed(KeyT key) const -> bool
    {
        return impl->isPressed(key);
    }

    void setStateFunc(const StateFunc & stateFunc)
    {
        impl->stateFunc_m = stateFunc;
    }

private:
    class Impl final
    {
    public:
        using ControllerMatrixImpl = typename ControllerMatrix::Impl;
        using ControllerMatrixImplIterator = typename std::list<ControllerMatrixImpl *>::iterator;

        auto addMatrixImpl(ControllerMatrixImpl * matrixImpl) -> ControllerMatrixImplIterator
        {
            return matrixImpls_m.insert(matrixImpls_m.end(), matrixImpl);
        }

        void removeMatrixImpl(ControllerMatrixImplIterator matrixImplIter)
        {
            matrixImpls_m.erase(matrixImplIter);
        }

        void reset()
        {
            for(ControllerMatrixImpl * i: matrixImpls_m) {
                i->reset();
            }
        }

        auto isPressed(KeyT key) const -> bool
        {
            for(const ControllerMatrixImpl * i: matrixImpls_m) {
                if(i->isPressed(key)) {
                    return true;
                }
            }
            return false;
        }

        void stateChanged(const ControllerMatrixImpl * matrixImpl, KeyT key, bool isPressed)
        {
            for(const ControllerMatrixImpl * i: matrixImpls_m) {
                if(i != matrixImpl && i->isPressed(key)) {
                    return;
                }
            }
            stateFunc_m(key, isPressed);
        }

        std::list<ControllerMatrixImpl *> matrixImpls_m;
        StateFunc stateFunc_m;
    };

    std::shared_ptr<Impl> impl;

    friend ControllerMatrix;
};

template <typename KeyT>
class ControllerMatrix final
{
public:
    using Controller = ::Controller<KeyT>;

    explicit ControllerMatrix(ControllerMatrix &&) = default;
    auto operator = (ControllerMatrix &&) -> ControllerMatrix & = default;

    explicit ControllerMatrix(Controller * controller):
        impl(std::make_unique<Impl>(controller))
    {}

    void reset()
    {
        impl->reset();
    }

    void setPressed(KeyT key, bool isPressed)
    {
        impl->setPressed(key, isPressed);
    }

    auto isPressed(KeyT key) const -> bool
    {
        return impl->isPressed(key);
    }

private:
    class Impl final
    {
    public:
        using ControllerImpl = typename Controller::Impl;

        explicit Impl(Controller * controller):
            controllerImpl_m(controller->impl),
            thisIter_m(controllerImpl_m->addMatrixImpl(this))
        {}

        ~Impl()
        {
            // FIXME: Key 'unpress' events doesn't generate here (but have to?)
            controllerImpl_m->removeMatrixImpl(thisIter_m);
        }

        void reset()
        {
            for(KeyT key: pressedKeys_m) {
                controllerImpl_m->stateChanged(this, key, false);
            }
            pressedKeys_m.clear();
        }

        void setPressed(KeyT key, bool isPressed)
        {
            bool isStateChanged = (isPressed ? pressedKeys_m.insert(key).second :
                                               pressedKeys_m.erase(key));
            if(isStateChanged) {
                controllerImpl_m->stateChanged(this, key, isPressed);
            }
        }

        auto isPressed(KeyT key) const -> bool
        {
            return (pressedKeys_m.find(key) != pressedKeys_m.end());
        }

        std::shared_ptr<ControllerImpl> controllerImpl_m;
        typename std::list<Impl *>::iterator thisIter_m;
        std::set<KeyT> pressedKeys_m;
    };

    std::unique_ptr<Impl> impl;

    friend Controller;
};

#endif // CONTROLLERS_H
