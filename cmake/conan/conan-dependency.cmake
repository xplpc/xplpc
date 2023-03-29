find_package(fmt REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    find_package(nlohmann_json REQUIRED CONFIG)
endif()

if(XPLPC_ENABLE_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        find_package(gtest REQUIRED CONFIG)
    endif()
endif()
