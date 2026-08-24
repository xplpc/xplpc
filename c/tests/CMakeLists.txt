set(XPLPC_TESTS_INC_FILES ${XPLPC_TESTS_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/DetachedAnswerMapping.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/GeneralTest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/ProxyCallbackRecorder.hpp"
    PARENT_SCOPE
)

set(XPLPC_TESTS_SRC_FILES ${XPLPC_TESTS_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CoreTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ClientTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConcurrencyTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/HeaderIsValidC.c"
    PARENT_SCOPE
)
