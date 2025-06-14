# log
set(SPDLOG_OPTIONS "SPDLOG_BUILD_PIC ON")
if(APPLE)
    list(APPEND SPDLOG_OPTIONS "SPDLOG_FWRITE_UNLOCKED OFF")
endif()

CPMAddPackage(
    NAME "spdlog"
    VERSION "1.15.2"
    GITHUB_REPOSITORY "gabime/spdlog"
    OPTIONS ${SPDLOG_OPTIONS}
)

target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

# serializer
if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    CPMAddPackage("gh:nlohmann/json@3.12.0")
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# test
if(XPLPC_BUILD_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        CPMAddPackage("gh:google/googletest@1.16.0")

        find_package(googletest REQUIRED)
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest GTest::gtest_main)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    endif()
endif()
