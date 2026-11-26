#pragma once

#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/Handle.hpp"

#include <optional>

namespace xplpc
{
namespace type
{

// Names a buffer registered by whoever owns the bytes, which is all that crosses the wire, so a view can never point a reader at memory nobody registered.
class DataView
{
public:
    DataView() = default;

    explicit DataView(core::Handle buffer)
        : buffer(buffer)
    {
    }

    core::Handle handle() const { return buffer; }

    // Answers the bytes while the owner keeps them registered, and nothing once it released them or when the view names nothing.
    std::optional<core::BufferSpan> bytes() const
    {
        if (!buffer)
        {
            return std::nullopt;
        }

        return core::BufferTable::shared().read(buffer);
    }

private:
    core::Handle buffer = 0;
};

} // namespace type
} // namespace xplpc
