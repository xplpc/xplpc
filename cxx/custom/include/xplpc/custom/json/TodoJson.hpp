#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "nlohmann/json.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/custom/json/JsonField.hpp"

namespace nlohmann
{
template <>
struct adl_serializer<Todo>
{
    static Todo from_json(const json &j)
    {
        return {
            JsonField::read<int64_t>(j, "id", 0),
            JsonField::read<std::string>(j, "title", {}),
            JsonField::read<std::string>(j, "body", {}),
            JsonField::read<std::unordered_map<std::string, std::string>>(j, "data", {}),
            JsonField::read<bool>(j, "done", false),
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
