#include "xplpc/type/Buffer.hpp"
#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/Router.hpp"

#include <utility>

namespace xplpc
{
namespace type
{

Buffer::Buffer(std::uint8_t *data, size_t size)
    : buffer(core::BufferTable::shared().add(core::Router::shared().native(), data, size))
{
}

Buffer::~Buffer()
{
    release();
}

Buffer::Buffer(Buffer &&other) noexcept
    : buffer(std::exchange(other.buffer, 0))
{
}

Buffer &Buffer::operator=(Buffer &&other) noexcept
{
    if (this != &other)
    {
        release();
        buffer = std::exchange(other.buffer, 0);
    }

    return *this;
}

DataView Buffer::view() const
{
    return DataView{buffer};
}

void Buffer::release()
{
    if (buffer)
    {
        core::BufferTable::shared().release(core::Router::shared().native(), buffer);
    }
}

} // namespace type
} // namespace xplpc
