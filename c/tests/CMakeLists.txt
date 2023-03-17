# files
set(XPLPC_TESTS_INC_FILES ${XPLPC_TESTS_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fixtures/GeneralTest.hpp"
    PARENT_SCOPE
)

set(XPLPC_TESTS_SRC_FILES ${XPLPC_TESTS_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CoreTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ClientTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConcurrencyTest.cpp"
    PARENT_SCOPE
)
