#pragma once

#include "xplpc/map/MappingItem.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"
#include "xplpc/serializer/Serializer.hpp"

#include <array>
#include <string>

namespace xplpc
{
namespace map
{

using namespace xplpc::serializer;
using namespace xplpc::message;

class Map
{
public:
    // The array is sized by the declared parameter types, so naming a parameter the signature does not carry is a build error rather than a mapping that ignores it.
    template <typename Tr, typename... Ts>
    static MappingItem create(const std::array<std::string, sizeof...(Ts)> &params, Target target)
    {
        // clang-format off
        return MappingItem{
            [params, target](core::Handle call, const std::string &data, const std::string &functionName) {
                Serializer::execute<Tr, Ts...>(call, data, params, functionName, target);
            },
        };
        // clang-format on
    }
};

} // namespace map
} // namespace xplpc
