# log
find_package(fmt REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

# serializer
if (XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    find_package(nlohmann_json REQUIRED CONFIG)
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# test
if(XPLPC_ENABLE_TESTS)
    if(${XPLPC_TARGET} MATCHES "^(cxx|c)-(static|shared)$")
        find_package(GTest REQUIRED CONFIG)
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest GTest::gtest_main)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    endif()
endif()
