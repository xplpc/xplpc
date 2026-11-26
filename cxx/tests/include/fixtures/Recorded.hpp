#pragma once

#include <cstdint>
#include <string>

// A record of the caller's own whose fields are written in the order every encoder agrees on.
class Recorded
{
public:
    std::int64_t count;
    std::string name;
};
