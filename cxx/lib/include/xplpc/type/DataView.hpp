#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace xplpc
{
namespace type
{

class DataView
{
public:
    DataView(uint8_t *ptr, size_t size)
        : _ptr(ptr)
        , _size(size)
    {
    }

    DataView(DataView &&) = default;

    DataView(const DataView &) = default;

    DataView &operator=(const DataView &rhs)
    {
        _ptr = rhs._ptr;
        _size = rhs._size;
        return *this;
    }

    void copy(uint8_t *targetPtr) const
    {
        std::copy(reinterpret_cast<uint8_t *>(_ptr), (reinterpret_cast<uint8_t *>(_ptr) + _size), reinterpret_cast<uint8_t *>(targetPtr));
    }

    uint8_t *ptr() const
    {
        return _ptr;
    }

    size_t size() const
    {
        return _size;
    }

private:
    uint8_t *_ptr;
    size_t _size;
};

} // namespace type
} // namespace xplpc
