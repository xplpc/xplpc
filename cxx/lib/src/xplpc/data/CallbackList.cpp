#include "xplpc/data/CallbackList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<CallbackList> CallbackList::instance = nullptr;

void CallbackList::add(const std::string &key, const std::function<void(const std::string &)> callback)
{
    list.set(key, callback);
}

void CallbackList::execute(const std::string &key, const std::string &data)
{
    const auto callback = list.get(key);

    if (callback.has_value())
    {
        list.remove(key);
        callback.value()(data);
    }
}

size_t CallbackList::count() const noexcept
{
    return list.count();
}

std::shared_ptr<CallbackList> CallbackList::shared()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<CallbackList>();
    }

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
