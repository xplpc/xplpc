#pragma once

#include "nlohmann/json.hpp"

// A consumer type whose from_json raises something that is not an exception, which is code this library did not write.
class RaisingAnswer
{
};

inline void from_json(const nlohmann::json &, RaisingAnswer &)
{
    throw 42;
}
