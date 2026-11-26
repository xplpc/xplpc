#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/util/Log.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace xplpc
{
namespace core
{

BufferTable &BufferTable::shared()
{
    // The table is never destroyed, for the same reason the router is not, since what keeps a buffer alive can belong to a host that is already gone at exit.
    static BufferTable *table = new BufferTable();
    return *table;
}

Handle BufferTable::add(Handle owner, std::uint8_t *data, size_t size, std::shared_ptr<void> keep)
{
    // A size past what one object can span is a negative length that crossed as unsigned, which every reader would walk past the end of the buffer.
    const auto address = reinterpret_cast<std::uintptr_t>(data);

    if (size > static_cast<size_t>((std::numeric_limits<std::ptrdiff_t>::max)()) || address > (std::numeric_limits<std::uintptr_t>::max)() - size)
    {
        util::Log::e("[BufferTable : add] The size of the buffer does not fit the memory it names");
        return 0;
    }

    // The owner is checked while the table is held, so a detach either finds the buffer when it releases what the layer owned or the layer is already gone here.
    // A sink of the logger is code of the host's own, which may reach back into the table, so a refusal is reported once the table is free.

    bool attached = false;
    Handle handle = 0;

    {
        std::lock_guard<std::mutex> lock(mutex);

        attached = Router::shared().attached(owner);

        if (attached)
        {
            handle = buffers.insert(Entry{owner, BufferSpan{data, size, std::move(keep)}});
        }
    }

    if (!attached)
    {
        util::Log::e("[BufferTable : add] The layer registering a buffer is not attached");
        return 0;
    }

    if (!handle)
    {
        util::Log::e("[BufferTable : add] Every handle a buffer can have is taken");
    }

    return handle;
}

std::optional<BufferSpan> BufferTable::read(Handle buffer) const
{
    std::optional<BufferSpan> span;

    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto entry = buffers.find(buffer);

        if (entry)
        {
            span = entry->span;
        }
    }

    if (!span)
    {
        util::Log::e("[BufferTable : read] The view names a buffer that is not registered");
    }

    return span;
}

bool BufferTable::release(Handle owner, Handle buffer)
{
    // What kept the bytes alive is let go outside the lock, since it can be a reference the host holds.

    std::optional<Entry> released;

    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto entry = buffers.find(buffer);

        if (entry && entry->owner == owner)
        {
            released = buffers.take(buffer);
        }
    }

    if (!released)
    {
        util::Log::e("[BufferTable : release] The buffer is not registered by the layer releasing it");
        return false;
    }

    return true;
}

void BufferTable::releaseOwnedBy(Handle owner)
{
    std::vector<Entry> released;

    {
        std::lock_guard<std::mutex> lock(mutex);

        // clang-format off
        const auto owned = buffers.select([owner](const Entry &entry) {
            return entry.owner == owner;
        });
        // clang-format on

        for (const auto handle : owned)
        {
            released.push_back(std::move(buffers.take(handle).value()));
        }
    }
}

size_t BufferTable::count() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return buffers.count();
}

} // namespace core
} // namespace xplpc
