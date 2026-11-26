#pragma once

#include "xplpc/data/MappingList.hpp"
#include "xplpc/map/Map.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"

#include <string>

// A mapping that never answers, so the only thing that can resolve a call to it is the caller detaching or cancelling.
class SilentMapping
{
public:
    static constexpr const char *name = "test.never";

    static void add()
    {
        xplpc::data::MappingList::shared()->add(name, xplpc::map::Map::create<std::string>({}, &target));
    }

private:
    static void target(const xplpc::message::Message &, const xplpc::message::Response &) {}
};
