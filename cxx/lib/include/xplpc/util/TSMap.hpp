#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>
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
    std::optional<Value> get(const Key &k) const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);

        auto it = list.find(k);

        if (it == list.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    template <class ValueC>
    void set(const Key &k, ValueC &&v)
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        list[k] = std::forward<ValueC>(v);
    }

    std::unordered_map<Key, Value> takeAll()
    {
        std::unique_lock<std::shared_mutex> lock(mtx);

        auto taken = std::move(list);
        list.clear();

        return taken;
    }

    size_t count() const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return list.size();
    }

    bool contains(const Key &k) const
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return list.find(k) != list.end();
    }

private:
    mutable std::shared_mutex mtx;
    std::unordered_map<Key, Value> list;
};

} // namespace util
} // namespace xplpc
