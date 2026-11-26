add_executable(xplpc-tests
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/AnswerWaiter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/CompletionRecorder.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/ConformanceCases.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/CoroutineTest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/FakeLayer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/GeneralTest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/HeldCalls.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/LogRecorder.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/RaisingAnswer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/Recorded.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/RecordedJson.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/ReenteringSink.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/ScopedLayer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/SilentMapping.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/SpyMapping.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/SuspendedCoroutine.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/VerifiedCall.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/ViewBytes.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AllTypesTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AwaitableTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BufferTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ClientTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CompileDefinitionsTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConcurrencyTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConformanceTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CoreTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/DataListTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RouterTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RoutingTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SerializerTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SlotTableTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/StressTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TodoTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/WindowsHeaderTest.cpp"
)

target_link_libraries(xplpc-tests PRIVATE xplpc GTest::gtest_main)

target_include_directories(xplpc-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(xplpc-tests PRIVATE XPLPC_CONFORMANCE_DIR="${XPLPC_ROOT_DIR}/conformance" XPLPC_VERSION="${XPLPC_VERSION}" XPLPC_VERSION_CODE="${XPLPC_VERSION_CODE}")

include(GoogleTest)
gtest_discover_tests(xplpc-tests DISCOVERY_MODE PRE_TEST)
