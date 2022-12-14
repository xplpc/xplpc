#pragma once

namespace xplpc
{
namespace core
{

class XPLPC
{
public:
    static void initialize();
    static bool isInitialized();

private:
    static bool initialized;
};

} // namespace core
} // namespace xplpc
