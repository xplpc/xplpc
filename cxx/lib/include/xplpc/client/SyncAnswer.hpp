#pragma once

#include <mutex>
#include <optional>
#include <string>

namespace xplpc
{
namespace client
{

class SyncAnswer
{
public:
    void set(const std::string &data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        value = data;
    }

    std::optional<std::string> get()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }

private:
    std::mutex mtx;
    std::optional<std::string> value;
};

} // namespace client
} // namespace xplpc
