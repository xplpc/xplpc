#pragma once

#include <atomic>

namespace xplpc
{
namespace core
{

class XPLPC
{
public:
    static void initialize();
    static void finalize();
    static bool isInitialized();

private:
    static std::atomic<bool> initialized;
};

} // namespace core
} // namespace xplpc
