#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "cvtstream.h"

class Compressor:
        public ICvt
{
public:
    static auto create() -> std::unique_ptr<Compressor>;

private:
    class Impl;
    explicit Compressor() = default;
};

#endif // COMPRESSOR_H
