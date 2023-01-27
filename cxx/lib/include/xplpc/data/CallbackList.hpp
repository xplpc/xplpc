#pragma once

#include "xplpc/util/TSMap.hpp"

#include <functional>
#include <memory>
#include <string>

namespace xplpc
{
namespace data
{

using namespace xplpc::util;

class CallbackList
{
public:
    CallbackList() = default;
    void add(const std::string &key, const std::function<void(const std::string &)> callback);
    void execute(const std::string &key, const std::string &data);
    size_t count();
    static std::shared_ptr<CallbackList> shared();

private:
    static std::shared_ptr<CallbackList> instance;
    TSMap<std::string, std::function<void(const std::string &)>> list;
};

} // namespace data
} // namespace xplpc
