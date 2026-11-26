#pragma once

#include "xplpc/core/Layer.hpp"

#include <string>

namespace xplpc
{
namespace layer
{

class NativeLayer : public core::Layer
{
public:
    void invoke(core::Handle call, const std::string &name, const std::string &request) override;

    // Declared by the library and defined by whoever links it, which registers the c++ mappings once per process.
    static void initializePlatform();
};

} // namespace layer
} // namespace xplpc
