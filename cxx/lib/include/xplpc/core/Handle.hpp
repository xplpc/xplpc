#pragma once

#include <cstdint>

namespace xplpc
{
namespace core
{

// A slot in the upper bits and its generation in the lower 32, so no valid handle is ever zero, and the slot stays below two to the twenty one so every handle is exact as a javascript number.
using Handle = std::uint64_t;

class HandleParts
{
public:
    static constexpr std::uint32_t firstGeneration = 1;
    static constexpr std::uint32_t slotLimit = std::uint32_t{1} << 21;

    static constexpr Handle compose(std::uint32_t slot, std::uint32_t generation)
    {
        return (static_cast<Handle>(slot) << 32) | generation;
    }

    static constexpr std::uint32_t slot(Handle handle)
    {
        return static_cast<std::uint32_t>(handle >> 32);
    }

    static constexpr std::uint32_t generation(Handle handle)
    {
        return static_cast<std::uint32_t>(handle);
    }

    static constexpr std::uint32_t next(std::uint32_t generation)
    {
        const auto following = generation + 1;
        return following == 0 ? firstGeneration : following;
    }
};

} // namespace core
} // namespace xplpc
