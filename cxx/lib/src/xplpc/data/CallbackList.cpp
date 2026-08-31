#include "xplpc/data/CallbackList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<CallbackList> CallbackList::instance = nullptr;
std::once_flag CallbackList::initInstanceFlag;

void CallbackList::add(const std::string &key, Callback callback)
{
    list.set(key, std::move(callback));
}

void CallbackList::execute(const std::string &key, const std::string &data)
{
    auto callback = list.take(key);

    if (callback.has_value())
    {
        callback.value()(data);
    }
}

void CallbackList::remove(const std::string &key)
{
    list.remove(key);
}

void CallbackList::clear()
{
    list.clear();
}

void CallbackList::answerAndClear(const std::string &data)
{
    // Every entry is taken before any of them is invoked, so a callback is free to reenter and none can be answered twice.
    auto taken = list.takeAll();

    for (auto &entry : taken)
    {
        entry.second(data);
    }
}

size_t CallbackList::count() const
{
    return list.count();
}

std::shared_ptr<CallbackList> CallbackList::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<CallbackList>(new CallbackList());
    });
    // clang-format on

    return instance;
}

#if defined(__EMSCRIPTEN__)
void CallbackList::executeFromJavascript(const std::string &key, const std::string &data)
{
    shared()->execute(key, data);
}
#endif

} // namespace data
} // namespace xplpc
