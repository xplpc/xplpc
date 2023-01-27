#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

class Todo
{
public:
    int64_t id;
    std::string title;
    std::string body;
    std::unordered_map<std::string, std::string> data;
    bool done;

    Todo(int64_t id_,
         std::string title_,
         std::string body_,
         std::unordered_map<std::string, std::string> data_,
         bool done_)
        : id(std::move(id_))
        , title(std::move(title_))
        , body(std::move(body_))
        , data(std::move(data_))
        , done(std::move(done_))
    {
    }
};
