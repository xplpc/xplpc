#pragma once

#include "xplpc/type/DataView.hpp"

#include <cstdint>
#include <vector>

// Reads what a view names, and a view naming nothing reads as no bytes at all.
class ViewBytes
{
public:
    static std::vector<uint8_t> of(const xplpc::type::DataView &view)
    {
        const auto bytes = view.bytes();

        if (!bytes)
        {
            return {};
        }

        return std::vector<uint8_t>(bytes->data, bytes->data + bytes->size);
    }
};
