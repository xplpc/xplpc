# log
CPMAddPackage(
    NAME "spdlog"
    VERSION "1.12.0"
    GITHUB_REPOSITORY "gabime/spdlog"
    OPTIONS
        "SPDLOG_BUILD_PIC ON"
)

target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

# serializer
if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    CPMAddPackage("gh:nlohmann/json@3.11.2")
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# test
if(XPLPC_ENABLE_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        CPMAddPackage("gh:google/googletest@1.13.0")

        find_package(googletest REQUIRED)
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest GTest::gtest_main)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    endif()
endif()
