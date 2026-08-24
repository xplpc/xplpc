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

    void copy(uint8_t *targetPtr) const
    {
        // A view that carries no address describes nothing to read, and a decoded one can arrive empty.

        if (!_ptr || !targetPtr)
        {
            return;
        }

        std::copy(_ptr, _ptr + _size, targetPtr);
    }

    uint8_t *ptr() const { return _ptr; }
    size_t size() const { return _size; }

private:
    uint8_t *_ptr;
    size_t _size;
};

} // namespace type
} // namespace xplpc
