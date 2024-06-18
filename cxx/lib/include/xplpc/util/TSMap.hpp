#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace xplpc
{
namespace util
{

template <class Key, class Value>
class TSMap
{
public:
    std::optional<Value> get(Key const &k) noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        auto it = list.find(k);
        if (it != list.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    template <class ValueC>
    void set(Key const &k, ValueC &&v) noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        list[k] = std::forward<ValueC>(v);
    }

    void remove(Key const &k) noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        list.erase(k);
    }

    size_t count() const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        return list.size();
    }

    bool contains(Key const &k) const noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        return list.find(k) != list.end();
    }

private:
    mutable std::mutex mtx;
    std::unordered_map<Key, Value> list;
};

} // namespace util
} // namespace xplpc
