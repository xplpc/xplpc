#pragma once

#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"

#include <functional>
#include <string>

namespace xplpc
{
namespace map
{

using namespace xplpc::message;

class MappingItem
{
public:
    MappingItem() = default;
    MappingItem(const std::function<void(const Message &, const Response)> &target, const std::function<void(const std::string &, const std::string &)> &executor);
    std::function<void(const Message &, const Response)> &getTarget();
    const std::function<void(const std::string &, const std::string &)> &getExecutor();

private:
    std::function<void(const Message &, const Response)> target;
    std::function<void(const std::string &, const std::string &)> executor;
};

} // namespace map
} // namespace xplpc
