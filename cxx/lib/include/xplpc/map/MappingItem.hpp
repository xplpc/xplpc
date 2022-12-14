#pragma once

#include "xplpc/message/Message.hpp"
#include "xplpc/message/Response.hpp"

#include <any>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace xplpc
{
namespace map
{

using namespace xplpc::message;

class MappingItem
{
public:
    MappingItem(const std::string &name, const std::function<Response(const Message &)> &target, const std::function<const std::string(const std::string &)> &executor);
    std::string &getName();
    std::function<Response(const Message &)> &getTarget();
    const std::function<const std::string(const std::string &)> &getExecutor();

private:
    std::string name;
    std::function<Response(const Message &)> target;
    std::function<const std::string(const std::string &)> executor;
};

} // namespace map
} // namespace xplpc
