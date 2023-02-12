#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "nlohmann/json.hpp"
#include "xplpc/custom/Todo.hpp"

namespace nlohmann
{
template <>
struct adl_serializer<Todo>
{
    static Todo from_json(const json &j)
    {
        return {
            j.contains("id") ? j["id"].get<int64_t>() : 0,
            j.contains("title") ? j["title"].get<std::string>() : "",
            j.contains("body") ? j["body"].get<std::string>() : "",
            j.contains("data") ? j["data"].get<std::unordered_map<std::string, std::string>>() : std::unordered_map<std::string, std::string>{},
            j.contains("done") ? j["done"].get<bool>() : false,
        };
    }

    static void to_json(json &j, Todo o)
    {
        j = nlohmann::json{
            {"id", o.id},
            {"title", o.title},
            {"body", o.body},
            {"data", o.data},
            {"done", o.done},
        };
    }
};
} // namespace nlohmann

#endif
