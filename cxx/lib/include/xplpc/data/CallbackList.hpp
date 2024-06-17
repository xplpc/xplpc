#pragma once

#include "xplpc/util/TSMap.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace xplpc
{
namespace data
{

using namespace xplpc::util;

class CallbackList
{
public:
    void add(const std::string &key, const std::function<void(const std::string &)> callback);
    void execute(const std::string &key, const std::string &data);
    size_t count() const noexcept;
    static std::shared_ptr<CallbackList> shared();

#if defined(__EMSCRIPTEN__)
    static void executeFromJavascript(const std::string &key, const std::string &data);
#endif

private:
    static std::shared_ptr<CallbackList> instance;
    static std::once_flag initInstanceFlag;
    TSMap<std::string, std::function<void(const std::string &)>> list;

    CallbackList() = default;
    CallbackList(const CallbackList &) = delete;
    CallbackList &operator=(const CallbackList &) = delete;
};

} // namespace data
} // namespace xplpc
