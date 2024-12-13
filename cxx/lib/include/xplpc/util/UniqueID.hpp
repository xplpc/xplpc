#pragma once

#include <atomic>
#include <string>
#include <cstdint>

namespace xplpc
{
namespace util
{

class UniqueID
{
public:
    static std::string generate();

private:
    static std::atomic<uint64_t> uid;
};

} // namespace util
} // namespace xplpc
