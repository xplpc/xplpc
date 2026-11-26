find_package(XCTest REQUIRED)

xctest_add_bundle(xplpc-tests xplpc
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/AllTypesTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/CancellationTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/ConformanceFiles.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/ConformanceTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/CoreTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/ClientTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/DataListTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/DataViewTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/SerializerTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/StressTest.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/TestSupport.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/TodoTest.swift"
)

xctest_add_test(XCTest.xplpc-tests xplpc-tests)

set_tests_properties(XCTest.xplpc-tests PROPERTIES
    ENVIRONMENT "XPLPC_CONFORMANCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../../conformance"
)

set_target_properties(xplpc-tests PROPERTIES
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES
    XCODE_ATTRIBUTE_SKIP_INSTALL NO
    XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0"
)

target_link_libraries(xplpc-tests PRIVATE xplpc ${FWK_FOUNDATION} "c++")
