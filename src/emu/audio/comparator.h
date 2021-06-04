#ifndef COMPARATOR_H
#define COMPARATOR_H

#include "cvtstream.h"

class Comparator:
        public ICvt
{
public:
    static auto create() -> std::unique_ptr<Comparator>;

private:
    class Impl;
    explicit Comparator() = default;
};

#endif // COMPARATOR_H
