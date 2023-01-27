# project
cmake_minimum_required(VERSION 3.20.0)
project(xplpc LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

# variables
set(XPLPC_ADD_CUSTOM_DATA OFF CACHE BOOL "Build Custom Data")
set(XPLPC_CUSTOM_DATA_PATH "cxx/custom" CACHE STRING "Custom Data Path")
set(XPLPC_CUSTOM_DATA_INCLUDE_PATH "cxx/custom/include" CACHE STRING "Custom Data Include Path")
set(XPLPC_ENABLE_SERIALIZER_FOR_JSON ON CACHE BOOL "Enable Serializer For Json")
set(XPLPC_ENABLE_TESTS OFF CACHE BOOL "Enable Tests")
set(XPLPC_INC_FILES "" CACHE INTERNAL "Include Files")
set(XPLPC_SRC_FILES "" CACHE INTERNAL "Source Files")
set(XPLPC_TESTS_INC_FILES "" CACHE INTERNAL "Tests Include Files")
set(XPLPC_TESTS_SRC_FILES "" CACHE INTERNAL "Tests Source Files")
set(XPLPC_TARGET "cxx" CACHE STRING "Target Name")
set(XPLPC_PUBLIC_HEADER_FILES "" CACHE INTERNAL "Public Header Files")
set(XPLPC_PROJECT_NAME ${PROJECT_NAME} CACHE INTERNAL "Project Name Suffix")
set(XPLPC_VERSION "1.0.0" CACHE STRING "Version Name")
set(XPLPC_VERSION_CODE "1" CACHE STRING "Version Code")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(XPLPC_ENABLE_LOG ON CACHE BOOL "Enable Log Messages")
else()
    set(XPLPC_ENABLE_LOG OFF CACHE BOOL "Enable Log Messages")
endif()

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin") # .exe and .dll
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib") # .so and .dylib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib") # .lib and .a

# dependency manager
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

# logger
CPMAddPackage("gh:gabime/spdlog@1.10.0")

if(XPLPC_ENABLE_LOG)
    add_compile_definitions(XPLPC_ENABLE_LOG)
endif()

# serializer
if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    CPMAddPackage("gh:nlohmann/json@3.11.2")
    add_compile_definitions(XPLPC_SERIALIZER_JSON)
else()
    message(FATAL_ERROR "You need one XPLPC serializer enabled")
endif()

# target
if(XPLPC_TARGET STREQUAL "")
    message(STATUS "XPLPC Target: Default")
else()
    message(STATUS "XPLPC Target: ${XPLPC_TARGET}")
endif()

# xplpc files
add_subdirectory(cxx/lib)

# custom data
if(XPLPC_ADD_CUSTOM_DATA)
    add_subdirectory(${XPLPC_CUSTOM_DATA_PATH})
endif()

# executable or library
if(XPLPC_TARGET STREQUAL "sample")
    add_compile_definitions(XPLPC_TARGET_SAMPLE)
    add_subdirectory(cxx/sample)

    set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
    add_executable(${XPLPC_PROJECT_NAME} "${XPLPC_ALL_FILES}")
elseif(XPLPC_TARGET STREQUAL "kotlin")
    add_compile_definitions(XPLPC_TARGET_KOTLIN)
    add_subdirectory(jni/lib)

    set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
    add_library(${XPLPC_PROJECT_NAME} SHARED "${XPLPC_ALL_FILES}")
    target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC jni/lib/include)
elseif(XPLPC_TARGET STREQUAL "swift")
    enable_language(Swift)
    set(CMAKE_Swift_LANGUAGE_VERSION 5)

    if (XPLPC_ENABLE_LOG)
        set(CMAKE_Swift_FLAGS "${CMAKE_Swift_FLAGS} -D XPLPC_ENABLE_LOG")
    endif()

    add_compile_definitions(XPLPC_TARGET_SWIFT)
    add_subdirectory(objc/lib)
    add_subdirectory(swift/lib)
    add_subdirectory(swift/support)

    set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
    add_library(${XPLPC_PROJECT_NAME} SHARED "${XPLPC_ALL_FILES}")
    target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC objc/lib/include)

    find_library(FWK_FOUNDATION Foundation)
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC ${${FWK_FOUNDATION}})

    set_target_properties(${XPLPC_PROJECT_NAME} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER "com.xplpc.library"
        VERSION ${XPLPC_VERSION} # "current version" in semantic format in Mach-O binary file
        SOVERSION ${XPLPC_VERSION_CODE} # "compatibility version" in semantic format in Mach-O binary file
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES
        XCODE_ATTRIBUTE_SKIP_INSTALL NO
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.xplpc.library"
        XCODE_ATTRIBUTE_DEFINES_MODULE YES
        XCODE_ATTRIBUTE_MODULEMAP_FILE "swift/support/module.modulemap"
        XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION YES
        PUBLIC_HEADER "${XPLPC_PUBLIC_HEADER_FILES}"
    )
elseif(XPLPC_TARGET STREQUAL "wasm")
    set(XPLPC_WASM_LINKER_FLAGS "--bind -sMALLOC=emmalloc -sWASM_BIGINT=1 -sALLOW_MEMORY_GROWTH=1 -fwasm-exceptions -sMODULARIZE=1 -sEXPORT_ES6=1")
    set(XPLPC_WASM_COMPILER_FLAGS "-fwasm-exceptions")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${XPLPC_WASM_LINKER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${XPLPC_WASM_COMPILER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XPLPC_WASM_COMPILER_FLAGS}")

    add_compile_definitions(XPLPC_TARGET_WASM)
    add_subdirectory(wasm/lib)

    set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
    add_executable(${XPLPC_PROJECT_NAME} "${XPLPC_ALL_FILES}")
else()
    set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
    add_library(${XPLPC_PROJECT_NAME} STATIC "${XPLPC_ALL_FILES}")
endif()

target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC cxx/lib/include)

if(XPLPC_ADD_CUSTOM_DATA)
    target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC ${XPLPC_CUSTOM_DATA_INCLUDE_PATH})
endif()

# dependencies
target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

# serializers
if (XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# tests
if(XPLPC_ENABLE_TESTS)
    if(XPLPC_TARGET STREQUAL "cxx")
        CPMAddPackage("gh:google/googletest@1.12.1#release-1.12.1")
        find_package(googletest REQUIRED)

        enable_testing()
        add_subdirectory(cxx/tests)
        set(XPLPC_TESTS_ALL_FILES ${XPLPC_TESTS_INC_FILES} ${XPLPC_TESTS_SRC_FILES})

        add_executable(${XPLPC_PROJECT_NAME}-tests "${XPLPC_TESTS_ALL_FILES}")

        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${XPLPC_PROJECT_NAME})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest GTest::gtest_main)
        target_include_directories(${XPLPC_PROJECT_NAME}-tests PRIVATE cxx/tests/include)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    elseif(XPLPC_TARGET STREQUAL "swift")
        find_library(FOUNDATION_LIBRARY Foundation)
        target_link_libraries(${XPLPC_PROJECT_NAME} PRIVATE ${FOUNDATION_LIBRARY})

        enable_testing()
        set_target_properties(${XPLPC_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_TESTABILITY YES)
        find_package(XCTest REQUIRED)
        add_subdirectory(swift/tests)
        set(XPLPC_TESTS_ALL_FILES ${XPLPC_TESTS_INC_FILES} ${XPLPC_TESTS_SRC_FILES})

        xctest_add_bundle(${XPLPC_PROJECT_NAME}-tests ${XPLPC_PROJECT_NAME} ${XPLPC_TESTS_ALL_FILES})
        xctest_add_test(XCTest.${XPLPC_PROJECT_NAME}-tests ${XPLPC_PROJECT_NAME}-tests)

        set_target_properties(${XPLPC_PROJECT_NAME}-tests PROPERTIES
            XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES
            XCODE_ATTRIBUTE_SKIP_INSTALL NO
            XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        )

        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${XPLPC_PROJECT_NAME})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${FOUNDATION_LIBRARY})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE "c++")
    endif()
endif()
