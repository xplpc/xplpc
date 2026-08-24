set(SPDLOG_OPTIONS "SPDLOG_BUILD_PIC ON")
if(APPLE)
    list(APPEND SPDLOG_OPTIONS "SPDLOG_FWRITE_UNLOCKED OFF")
endif()

CPMAddPackage(
    NAME "spdlog"
    VERSION "1.17.0"
    GITHUB_REPOSITORY "gabime/spdlog"
    OPTIONS ${SPDLOG_OPTIONS}
)

target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    CPMAddPackage("gh:nlohmann/json@3.12.0")
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

if(XPLPC_BUILD_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        # A system include directory is searched after /usr/local/include, so a googletest installed there is what a local build compiles against while this one is linked.
        # The marking comes from two places, the shorthand form of CPMAddPackage and googletest marking its own interface, so both are cleared.
        CPMAddPackage(
            NAME googletest
            GITHUB_REPOSITORY google/googletest
            VERSION 1.18.0
            EXCLUDE_FROM_ALL YES
            SYSTEM NO
        )

        foreach(gtest_target gtest gtest_main gmock gmock_main)
            if(TARGET ${gtest_target})
                set_target_properties(${gtest_target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "")
            endif()
        endforeach()

        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest_main)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    endif()
endif()
