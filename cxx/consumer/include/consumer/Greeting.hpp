#pragma once

#include "xplpc/xplpc.hpp"

#include <string>

namespace consumer
{

class Greeting
{
public:
    static void target(const xplpc::message::Message &message, const xplpc::message::Response &response)
    {
        response("Hello, " + message.get<std::string>("name").value_or("stranger"));
    }
};

} // namespace consumer
