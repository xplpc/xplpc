#pragma once

#include <string>

namespace xplpc
{
namespace layer
{

// A copy the host owns and releases with xplpc_free, since it reads the bytes after the frame that produced them is gone.
class OwnedBytes
{
public:
    // Answers null only when there was no memory, and an empty value still gets a valid address since asking for zero bytes may answer null.
    static char *copy(const std::string &value);
};

} // namespace layer
} // namespace xplpc
