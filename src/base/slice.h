#ifndef SLICE_H
#define SLICE_H

#include <stddef.h>
#include <algorithm>

template <typename T>
class Slice
{
public:
    explicit Slice(T * data = nullptr, size_t count = 0):
        data_m(data), bytes_m(count * sizeof(T))
    {}

    auto data() const -> T *
    {
        return data_m;
    }
    
    auto count() const -> size_t
    {
        return (bytes_m / sizeof(T));
    }

    auto expand(size_t count = 1) -> size_t
    {
        bytes_m += (count * sizeof(T));
        return count;
    }

    auto reduce(size_t count = 1) -> size_t
    {
        count = std::min(count, bytes_m / sizeof(T));
        bytes_m -= (count * sizeof(T));
        return count;
    }

    auto advance(size_t count = 1) -> size_t
    {
        count = std::min(count, bytes_m / sizeof(T));
        data_m += count;
        bytes_m -= (count * sizeof(T));
        return count;
    }

    auto isConsistent() const -> bool
    {
        return ((bytes_m % sizeof(T)) == 0);
    }

private:
    T * data_m;
    size_t bytes_m;
};

#endif // SLICE_H
