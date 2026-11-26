# The default pattern keeps a map per thread, and mingw frees thread storage before it destroys what that storage holds, so a thread that logged corrupts the heap as it exits.
set(SPDLOG_OPTIONS "SPDLOG_BUILD_PIC ON" "SPDLOG_NO_TLS ON")

if(APPLE)
    list(APPEND SPDLOG_OPTIONS "SPDLOG_FWRITE_UNLOCKED OFF")
endif()

CPMAddPackage(
    NAME "spdlog"
    VERSION "1.17.0"
    GITHUB_REPOSITORY "gabime/spdlog"
    OPTIONS ${SPDLOG_OPTIONS}
)

CPMAddPackage("gh:nlohmann/json@3.12.0")

# The headers include both, so whoever links the library is given them.
target_link_libraries(xplpc PUBLIC spdlog::spdlog nlohmann_json::nlohmann_json)

if(XPLPC_BUILD_TESTS AND XPLPC_TARGET MATCHES "^(cxx-static|c-static|c-shared)$")
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
endif()
