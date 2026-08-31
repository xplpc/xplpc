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

using Target = std::function<void(const Message &, const Response &)>;
using Executor = std::function<void(const std::string &key, const std::string &data, const std::string &functionName)>;

class MappingItem
{
public:
    // The keyed map this is stored in assigns through the subscript, which builds a value before it overwrites one.
    MappingItem() = default;
    explicit MappingItem(Executor executor);

    const Executor &getExecutor() const;

private:
    Executor executor;
};

} // namespace map
} // namespace xplpc
