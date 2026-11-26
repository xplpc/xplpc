#pragma once

#include "fixtures/Recorded.hpp"

#include "nlohmann/json.hpp"

namespace nlohmann
{

template <>
struct adl_serializer<Recorded>
{
    static void to_json(json &j, const Recorded &recorded)
    {
        j = nlohmann::json{
            {"count", recorded.count},
            {"name", recorded.name},
        };
    }
};

} // namespace nlohmann
