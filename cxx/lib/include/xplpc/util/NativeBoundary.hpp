#pragma once

#include "xplpc/util/Log.hpp"

#include <exception>

namespace xplpc
{
namespace util
{

class NativeBoundary
{
public:
    template <typename Body>
    static void run(const char *source, Body &&body)
    {
        // clang-format off
        runAnswering(source, false, [&body]() {
            body();
            return true;
        });
        // clang-format on
    }

    template <typename Value, typename Body>
    static Value runAnswering(const char *source, Value fallbackValue, Body &&body)
    {
        // A c++ exception leaving an extern "C" function, a jni entry point or an objective-c method is undefined in all three abis, so it stops here and is reported.

        try
        {
            return body();
        }
        catch (const std::exception &e)
        {
            Log::e("[{}] The call raised and was stopped at the native boundary", source);
            Log::d("[{}] The call raised and was stopped at the native boundary: {}", source, e.what());
        }
        catch (...)
        {
            Log::e("[{}] The call raised something that is not an exception and was stopped at the native boundary", source);
        }

        return fallbackValue;
    }
};

} // namespace util
} // namespace xplpc
