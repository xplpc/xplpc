#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/core/SlotTable.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace xplpc
{
namespace core
{

// The bytes a view names and what keeps them alive, so a reader holding the span keeps a buffer the host owns from being freed under it.
struct BufferSpan
{
    std::uint8_t *data;
    size_t size;
    std::shared_ptr<void> keep;
};

// Every buffer that crosses the bridge as a handle, so a view that was released, was never registered or names another one is refused instead of read.
class BufferTable
{
public:
    static BufferTable &shared();

    // Registers bytes for an attached layer, which keeps them valid and in place until it releases them, and what keeps them alive is held until then.
    Handle add(Handle owner, std::uint8_t *data, size_t size, std::shared_ptr<void> keep = nullptr);

    std::optional<BufferSpan> read(Handle buffer) const;

    // Answers false when the buffer is not registered or belongs to another layer, since only the side that registered a buffer may release it.
    bool release(Handle owner, Handle buffer);

    void releaseOwnedBy(Handle owner);

    size_t count() const;

private:
    struct Entry
    {
        Handle owner;
        BufferSpan span;
    };

    mutable std::mutex mutex;
    SlotTable<Entry> buffers;

    BufferTable() = default;
    BufferTable(const BufferTable &) = delete;
    BufferTable &operator=(const BufferTable &) = delete;
};

} // namespace core
} // namespace xplpc
