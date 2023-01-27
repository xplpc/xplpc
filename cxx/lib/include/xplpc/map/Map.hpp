#pragma once

#include "xplpc/map/MappingItem.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"
#include "xplpc/serializer/Serializer.hpp"

#include <functional>
#include <string>
#include <vector>

namespace xplpc
{
namespace map
{

using namespace xplpc::map;
using namespace xplpc::serializer;
using namespace xplpc::data;
using namespace xplpc::message;

class Map
{
public:
    template <typename Tr, typename... Ts>
    static MappingItem create(const std::vector<std::string> &params, const std::function<void(const Message &, const Response)> &target)
    {
        return MappingItem{
            target,
            [=](const std::string &key, const std::string &data)
            {
                Serializer::execute<Tr, Ts...>(key, data, params);
            },
        };
    }
};

} // namespace map
} // namespace xplpc
