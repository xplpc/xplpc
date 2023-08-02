# project
cmake_minimum_required(VERSION 3.20.0)
project(xplpc LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

# policies
cmake_policy(SET CMP0054 NEW) # only interpret if() arguments as variables or keywords when unquoted
cmake_policy(SET CMP0076 NEW) # target_sources() command converts relative paths to absolute
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW) # msvc runtime library flags are selected by an abstraction
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW) # set the timestamps of all extracted contents to the time of the extraction

# settings
set(XPLPC_DEPENDENCY_TOOL_VALUES "cpm" "conan")

if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# options
set(XPLPC_ADD_CUSTOM_DATA OFF CACHE BOOL "Build Custom Data")
set(XPLPC_CUSTOM_DATA_PATH "cxx/custom" CACHE STRING "Custom Data Path")
set(XPLPC_CUSTOM_DATA_INCLUDE_PATH "cxx/custom/include" CACHE STRING "Custom Data Include Path")
set(XPLPC_ENABLE_SERIALIZER_FOR_JSON ON CACHE BOOL "Enable Serializer For Json")
set(XPLPC_ENABLE_TESTS OFF CACHE BOOL "Enable Tests")
set(XPLPC_ENABLE_SAMPLES OFF CACHE BOOL "Enable Samples")
set(XPLPC_ENABLE_INTERFACE OFF CACHE BOOL "Enable Interface")
set(XPLPC_INC_FILES "" CACHE INTERNAL "Include Files")
set(XPLPC_SRC_FILES "" CACHE INTERNAL "Source Files")
set(XPLPC_TESTS_INC_FILES "" CACHE INTERNAL "Tests Include Files")
set(XPLPC_TESTS_SRC_FILES "" CACHE INTERNAL "Tests Source Files")
set(XPLPC_TARGET "" CACHE STRING "Target Name")
set(XPLPC_PUBLIC_HEADER_FILES "" CACHE INTERNAL "Public Header Files")
set(XPLPC_PROJECT_NAME ${PROJECT_NAME} CACHE INTERNAL "Project Name Suffix")
set(XPLPC_VERSION "1.0.0" CACHE STRING "Version Name")
set(XPLPC_VERSION_CODE "1" CACHE STRING "Version Code")
set(XPLPC_DEPENDENCY_TOOL "cpm" CACHE STRING "Dependency Tool")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(XPLPC_ENABLE_LOG ON CACHE BOOL "Enable Log Messages")
else()
    set(XPLPC_ENABLE_LOG OFF CACHE BOOL "Enable Log Messages")
endif()

# validate dependency tool
if(NOT XPLPC_DEPENDENCY_TOOL IN_LIST XPLPC_DEPENDENCY_TOOL_VALUES)
    message(FATAL_ERROR "Dependency tool need be one of: ${XPLPC_DEPENDENCY_TOOL_VALUES}")
endif()

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin") # .exe and .dll
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "bin") # .exe and .dll
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "bin") # .exe and .dll

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib") # .so and .dylib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "lib") # .so and .dylib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "lib") # .so and .dylib

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib") # .lib and .a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "lib") # .lib and .a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "lib") # .lib and .a

# download dependency tool
if (XPLPC_DEPENDENCY_TOOL STREQUAL "cpm")
    include(${CMAKE_SOURCE_DIR}/cmake/cpm/cpm-downloader.cmake)
endif()

# setup all dependencies
if (XPLPC_DEPENDENCY_TOOL STREQUAL "cpm")
    include(${CMAKE_SOURCE_DIR}/cmake/cpm/cpm-dependency.cmake)
elseif (XPLPC_DEPENDENCY_TOOL STREQUAL "conan")
    include(${CMAKE_SOURCE_DIR}/cmake/conan/conan-dependency.cmake)
endif()

# setup log
if(XPLPC_ENABLE_LOG)
    add_compile_definitions(XPLPC_ENABLE_LOG)
endif()

# setup serializer
if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    add_compile_definitions(XPLPC_SERIALIZER_JSON)
else()
    message(FATAL_ERROR "You need enable one serializer")
endif()

# setup target
if(XPLPC_TARGET STREQUAL "")
    message(FATAL_ERROR "You need set the target")
else()
    message(STATUS "XPLPC Target: ${XPLPC_TARGET}")
endif()

# xplpc files
add_subdirectory(cxx/lib)

# xplpc interface
if(XPLPC_ENABLE_INTERFACE)
    message(STATUS "XPLPC Interface: Enabled")
    add_subdirectory(c/lib)
else()
    message(STATUS "XPLPC Interface: Disabled")
endif()

# custom data
if(XPLPC_ADD_CUSTOM_DATA)
    message(STATUS "XPLPC Custom Data: ${XPLPC_CUSTOM_DATA_PATH}")
    add_subdirectory(${XPLPC_CUSTOM_DATA_PATH})
endif()

# target data
if (XPLPC_ENABLE_SAMPLES)
    if(XPLPC_TARGET STREQUAL "cxx-static" OR XPLPC_TARGET STREQUAL "cxx-shared")
        add_compile_definitions(XPLPC_TARGET_CXX)
        add_subdirectory(cxx/sample)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_executable(${XPLPC_PROJECT_NAME} "${XPLPC_ALL_FILES}")
    elseif(XPLPC_TARGET STREQUAL "c-static" OR XPLPC_TARGET STREQUAL "c-shared")
        add_compile_definitions(XPLPC_TARGET_C)
        add_subdirectory(c/sample)
        add_subdirectory(c/lib)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_executable(${XPLPC_PROJECT_NAME} "${XPLPC_ALL_FILES}")
        target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC c/lib/include)
    endif()
else()
    if(XPLPC_TARGET STREQUAL "cxx-static")
        add_compile_definitions(XPLPC_TARGET_CXX)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_library(${XPLPC_PROJECT_NAME} STATIC "${XPLPC_ALL_FILES}")
    elseif(XPLPC_TARGET STREQUAL "cxx-shared")
        add_compile_definitions(XPLPC_TARGET_CXX)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_library(${XPLPC_PROJECT_NAME} SHARED "${XPLPC_ALL_FILES}")
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
        target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC ${FWK_FOUNDATION})

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
        set(XPLPC_WASM_LINKER_FLAGS "--bind -sMALLOC=emmalloc -sWASM_BIGINT=1 -sALLOW_MEMORY_GROWTH=1 -fwasm-exceptions -sMODULARIZE=1 -sEXPORT_ES6=1 -s \"EXPORTED_FUNCTIONS=['_malloc', '_free']\"")
        set(XPLPC_WASM_COMPILER_FLAGS "-fwasm-exceptions")

        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${XPLPC_WASM_LINKER_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${XPLPC_WASM_COMPILER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XPLPC_WASM_COMPILER_FLAGS}")

        add_compile_definitions(XPLPC_TARGET_WASM)
        add_subdirectory(wasm/lib)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_executable(${XPLPC_PROJECT_NAME} "${XPLPC_ALL_FILES}")
    elseif(XPLPC_TARGET STREQUAL "c-static")
        add_compile_definitions(XPLPC_TARGET_C)
        add_subdirectory(c/lib)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_library(${XPLPC_PROJECT_NAME} STATIC "${XPLPC_ALL_FILES}")
        target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC c/lib/include)
    elseif(XPLPC_TARGET STREQUAL "c-shared")
        add_compile_definitions(XPLPC_TARGET_C)
        add_subdirectory(c/lib)

        set(XPLPC_ALL_FILES ${XPLPC_INC_FILES} ${XPLPC_SRC_FILES})
        add_library(${XPLPC_PROJECT_NAME} SHARED "${XPLPC_ALL_FILES}")
        target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC c/lib/include)
    else()
        message(FATAL_ERROR "Target is invalid: ${XPLPC_TARGET}")
    endif()
endif()

# xplpc interface include path
if(XPLPC_ENABLE_INTERFACE)
    target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC c/lib/include)
endif()

# cxx include path
target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC cxx/lib/include)

# custom data
if(XPLPC_ADD_CUSTOM_DATA)
    target_include_directories(${XPLPC_PROJECT_NAME} PUBLIC ${XPLPC_CUSTOM_DATA_INCLUDE_PATH})
endif()

# dependencies
target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC spdlog::spdlog)

# android dependencies
if(XPLPC_TARGET STREQUAL "kotlin")
    if (CMAKE_SYSTEM_NAME MATCHES "Android")
        find_library(ANDROID_LIB android)
        find_library(ANDROID_LOG_LIB log)
        target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC ${ANDROID_LIB} ${ANDROID_LOG_LIB})
    else()
        set(JAVA_AWT_LIBRARY NotNeeded)
        set(JAVA_JVM_LIBRARY NotNeeded)
        set(JAVA_INCLUDE_PATH2 NotNeeded)
        set(JAVA_AWT_INCLUDE_PATH NotNeeded)
        find_package(JNI REQUIRED)
        target_link_libraries(${XPLPC_PROJECT_NAME} PRIVATE JNI::JNI)
    endif()
endif()

# serializers
if (XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# tests
if(XPLPC_ENABLE_TESTS)
    if(XPLPC_TARGET STREQUAL "cxx-static" OR XPLPC_TARGET STREQUAL "cxx-shared")
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
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        )

        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${XPLPC_PROJECT_NAME})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${FOUNDATION_LIBRARY})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE "c++")
    elseif(XPLPC_TARGET STREQUAL "c-static" OR XPLPC_TARGET STREQUAL "c-shared")
        enable_testing()
        add_subdirectory(c/tests)
        set(XPLPC_TESTS_ALL_FILES ${XPLPC_TESTS_INC_FILES} ${XPLPC_TESTS_SRC_FILES})

        add_executable(${XPLPC_PROJECT_NAME}-tests "${XPLPC_TESTS_ALL_FILES}")

        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE ${XPLPC_PROJECT_NAME})
        target_link_libraries(${XPLPC_PROJECT_NAME}-tests PRIVATE GTest::gtest GTest::gtest_main)
        target_include_directories(${XPLPC_PROJECT_NAME}-tests PRIVATE c/tests/include)

        include(GoogleTest)
        gtest_add_tests(TARGET ${XPLPC_PROJECT_NAME}-tests)
    endif()
endif()
