CPMAddPackage("gh:gabime/spdlog@1.12.0")

if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    CPMAddPackage("gh:nlohmann/json@3.11.2")
endif()

if(XPLPC_ENABLE_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        CPMAddPackage("gh:google/googletest@1.13.0")
        find_package(googletest REQUIRED)
    endif()
endif()
