find_package(spdlog REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

# The headers include both, so whoever links the library is given them.
target_link_libraries(xplpc PUBLIC spdlog::spdlog nlohmann_json::nlohmann_json)

# Spdlog is built without per thread storage, and the recipe does not hand the definition on, so the headers are told here.
target_compile_definitions(xplpc PUBLIC SPDLOG_NO_TLS)

if(XPLPC_BUILD_TESTS AND XPLPC_TARGET MATCHES "^(cxx-static|c-static|c-shared)$")
    find_package(GTest REQUIRED CONFIG)
endif()
