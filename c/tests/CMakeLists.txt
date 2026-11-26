add_executable(xplpc-tests
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/GeneralTest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/HeldResponders.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/Requests.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/TestHost.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BufferTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ClientTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConcurrencyTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/HeaderIsValidC.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LifecycleTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RoutingTest.cpp"
)

target_link_libraries(xplpc-tests PRIVATE xplpc GTest::gtest_main)
target_include_directories(xplpc-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(GoogleTest)
gtest_add_tests(TARGET xplpc-tests)
