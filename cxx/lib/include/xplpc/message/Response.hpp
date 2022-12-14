#pragma once

#include <any>
#include <string>
#include <utility>

namespace xplpc
{
namespace message
{

class Response
{
public:
    Response(std::any value) { this->value = value; }
    std::any &getValue() { return this->value; }
    static Response none() { return Response({}); }
    static Response str(const std::string &value) { return Response(value); }
    bool hasValue() { return this->value.has_value(); }

private:
    std::any value;
};

} // namespace message
} // namespace xplpc
