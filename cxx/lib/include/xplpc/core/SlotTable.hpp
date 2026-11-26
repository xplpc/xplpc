#pragma once

#include "xplpc/core/Handle.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace xplpc
{
namespace core
{

// Hands out a handle per value and refuses a handle whose slot was released, including after the slot was reused.
template <typename T>
class SlotTable
{
public:
    // Answers zero when every slot a handle can name is taken.
    Handle insert(T value)
    {
        std::uint32_t slot = 0;

        if (freeSlots.empty() && slots.size() == HandleParts::slotLimit)
        {
            return 0;
        }

        if (freeSlots.empty())
        {
            slot = static_cast<std::uint32_t>(slots.size());
            slots.emplace_back();
        }
        else
        {
            slot = freeSlots.back();
            freeSlots.pop_back();
        }

        auto &entry = slots[slot];
        entry.value = std::move(value);
        live++;

        return HandleParts::compose(slot, entry.generation);
    }

    T *find(Handle handle)
    {
        return holds(handle) ? &slots[HandleParts::slot(handle)].value.value() : nullptr;
    }

    const T *find(Handle handle) const
    {
        return holds(handle) ? &slots[HandleParts::slot(handle)].value.value() : nullptr;
    }

    std::optional<T> take(Handle handle)
    {
        if (!holds(handle))
        {
            return std::nullopt;
        }

        auto &entry = slots[HandleParts::slot(handle)];

        auto value = std::move(entry.value);
        entry.value.reset();
        entry.generation = HandleParts::next(entry.generation);

        freeSlots.push_back(HandleParts::slot(handle));
        live--;

        return value;
    }

    template <typename F>
    std::vector<Handle> select(F predicate) const
    {
        std::vector<Handle> handles;

        for (std::uint32_t slot = 0; slot < slots.size(); slot++)
        {
            const auto &entry = slots[slot];

            if (entry.value.has_value() && predicate(entry.value.value()))
            {
                handles.push_back(HandleParts::compose(slot, entry.generation));
            }
        }

        return handles;
    }

    size_t count() const
    {
        return live;
    }

private:
    struct Entry
    {
        std::uint32_t generation = HandleParts::firstGeneration;
        std::optional<T> value;
    };

    std::vector<Entry> slots;
    std::vector<std::uint32_t> freeSlots;
    size_t live = 0;

    bool holds(Handle handle) const
    {
        const auto slot = HandleParts::slot(handle);

        if (slot >= slots.size())
        {
            return false;
        }

        const auto &entry = slots[slot];
        return entry.value.has_value() && entry.generation == HandleParts::generation(handle);
    }
};

} // namespace core
} // namespace xplpc
