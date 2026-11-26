#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/type/DataView.hpp"

#include <cstddef>
#include <cstdint>

namespace xplpc
{
namespace type
{

// Registers bytes the c++ side owns for as long as this lives, and the caller keeps them valid and in place for all of that time.
class Buffer
{
public:
    Buffer(std::uint8_t *data, size_t size);
    ~Buffer();

    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&other) noexcept;

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    DataView view() const;

private:
    core::Handle buffer;

    void release();
};

} // namespace type
} // namespace xplpc
