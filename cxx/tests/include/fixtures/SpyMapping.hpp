#pragma once

#include "xplpc/data/MappingList.hpp"
#include "xplpc/map/Map.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"

#include <atomic>
#include <string>

// A mapping that only counts how often it ran, which is what tells a refused request from one that was routed.
class SpyMapping
{
public:
    static constexpr const char *name = "test.spy";

    static void add()
    {
        runs().store(0);
        xplpc::data::MappingList::shared()->add(name, xplpc::map::Map::create<std::string>({}, &target));
    }

    static int count() { return runs().load(); }

private:
    static std::atomic<int> &runs()
    {
        static std::atomic<int> value{0};
        return value;
    }

    static void target(const xplpc::message::Message &, const xplpc::message::Response &r)
    {
        runs().fetch_add(1);
        r(std::string("ran"));
    }
};
