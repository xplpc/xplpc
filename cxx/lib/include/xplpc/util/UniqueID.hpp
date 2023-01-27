#pragma once

#include <atomic>
#include <string>

namespace xplpc
{
namespace util
{

class UniqueID
{
public:
    static std::string generate();

private:
    static std::atomic<std::uint64_t> uid;
};

} // namespace util
} // namespace xplpc
