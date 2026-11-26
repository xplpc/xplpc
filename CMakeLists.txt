cmake_minimum_required(VERSION 3.25)
project(xplpc LANGUAGES CXX)

set(XPLPC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

# Only what XPLPC_API and JNIEXPORT mark leaves the library, and this is set before any dependency is added so their objects are hidden too.
# These are directory variables, so a project that adds this one as a subdirectory keeps its own.
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(XPLPC_TARGET_VALUES "cxx-static" "c-static" "c-shared" "kotlin" "swift" "wasm")
set(XPLPC_DEPENDENCY_TOOL_VALUES "cpm" "conan")
set(XPLPC_SANITIZER_VALUES "none" "address" "thread" "undefined")

set(XPLPC_TARGET "cxx-static" CACHE STRING "What the library is built for")
set_property(CACHE XPLPC_TARGET PROPERTY STRINGS ${XPLPC_TARGET_VALUES})
set(XPLPC_DEPENDENCY_TOOL "cpm" CACHE STRING "Dependency Tool")
set(XPLPC_SANITIZER "none" CACHE STRING "Sanitizer")
set(XPLPC_VERSION "1.0.0" CACHE STRING "Version Name")
set(XPLPC_VERSION_CODE "1" CACHE STRING "Version Code")
set(XPLPC_CUSTOM_DATA_PATH "${XPLPC_ROOT_DIR}/cxx/custom" CACHE PATH "Directory whose CMakeLists.txt adds mappings to the library")

option(XPLPC_ADD_CUSTOM_DATA "Build the mappings in XPLPC_CUSTOM_DATA_PATH into the library" OFF)
option(XPLPC_ENABLE_SERIALIZER_FOR_JSON "Enable Serializer For Json" ON)
option(XPLPC_ENABLE_INTERFACE "Build the C interface" OFF)
option(XPLPC_BUILD_TESTS "Build Tests" OFF)
option(XPLPC_BUILD_SAMPLE "Build Sample" OFF)

if(NOT XPLPC_TARGET IN_LIST XPLPC_TARGET_VALUES)
    message(FATAL_ERROR "Target need be one of: ${XPLPC_TARGET_VALUES}")
endif()

if(NOT XPLPC_DEPENDENCY_TOOL IN_LIST XPLPC_DEPENDENCY_TOOL_VALUES)
    message(FATAL_ERROR "Dependency tool need be one of: ${XPLPC_DEPENDENCY_TOOL_VALUES}")
endif()

if(NOT XPLPC_SANITIZER IN_LIST XPLPC_SANITIZER_VALUES)
    message(FATAL_ERROR "Sanitizer need be one of: ${XPLPC_SANITIZER_VALUES}")
endif()

if(NOT XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    message(FATAL_ERROR "A serializer has to be enabled, and json is the only one")
endif()

message(STATUS "XPLPC Target: ${XPLPC_TARGET}")

# A project consuming this one decides its own build type and where its binaries land.
if(PROJECT_IS_TOP_LEVEL)
    if(NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib")

    # A multi configuration generator writes every configuration it is not told about into a directory of its own, which is not where the tasks read.
    foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "bin")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "lib")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "lib")
    endforeach()
endif()

# The swift driver rejects the clang spelling of the sanitizer and warning flags, so those reach the c family alone.
set(XPLPC_C_FAMILY_LANGUAGES "$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>")

if(NOT XPLPC_SANITIZER STREQUAL "none")
    message(STATUS "XPLPC Sanitizer: ${XPLPC_SANITIZER}")

    add_compile_options(
        "$<${XPLPC_C_FAMILY_LANGUAGES}:-fsanitize=${XPLPC_SANITIZER}>"
        "$<${XPLPC_C_FAMILY_LANGUAGES}:-fno-omit-frame-pointer>"
        "$<${XPLPC_C_FAMILY_LANGUAGES}:-g>"
    )

    add_link_options(-fsanitize=${XPLPC_SANITIZER})

    if(XPLPC_SANITIZER STREQUAL "undefined")
        add_compile_options("$<${XPLPC_C_FAMILY_LANGUAGES}:-fno-sanitize-recover=all>")
        add_link_options(-fno-sanitize-recover=all)
    endif()
endif()

if(XPLPC_TARGET MATCHES "^c-(static|shared)$")
    set(XPLPC_ENABLE_INTERFACE ON)
endif()

if(XPLPC_TARGET STREQUAL "swift")
    enable_language(Swift)
    set(CMAKE_Swift_LANGUAGE_VERSION 6)
endif()

if(XPLPC_TARGET STREQUAL "wasm")
    # Every object has to agree on how exceptions unwind, the dependencies included, so this reaches the whole directory.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fwasm-exceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fwasm-exceptions")
endif()

if(XPLPC_TARGET STREQUAL "cxx-static" OR XPLPC_TARGET STREQUAL "c-static")
    add_library(xplpc STATIC)
elseif(XPLPC_TARGET STREQUAL "wasm")
    add_executable(xplpc)
else()
    add_library(xplpc SHARED)
endif()

if(NOT XPLPC_TARGET STREQUAL "wasm")
    add_library(xplpc::xplpc ALIAS xplpc)
endif()

# The xcode generator reads the optimization and the debug information of a target mixing swift and c++ from the swift flags, which would build the c++ of a release at -O1 and without what a dsym is made of.
if(XPLPC_TARGET STREQUAL "swift")
    set_target_properties(xplpc PROPERTIES
        XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=Release] 3
        XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=RelWithDebInfo] 2
        XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=MinSizeRel] s
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS YES
    )
endif()

# The headers read the serializer and the standard, so whoever includes them is given both.
target_compile_features(xplpc PUBLIC cxx_std_20)
target_compile_definitions(xplpc PUBLIC XPLPC_SERIALIZER_JSON)
target_compile_definitions(xplpc PRIVATE XPLPC_VERSION="${XPLPC_VERSION}" XPLPC_VERSION_CODE="${XPLPC_VERSION_CODE}")

# A signed comparison or a parameter nobody reads is how a real defect first shows, so the project is built with them visible.
if(MSVC)
    target_compile_options(xplpc PRIVATE "$<${XPLPC_C_FAMILY_LANGUAGES}:/W4>")
else()
    target_compile_options(xplpc PRIVATE "$<${XPLPC_C_FAMILY_LANGUAGES}:-Wall>" "$<${XPLPC_C_FAMILY_LANGUAGES}:-Wextra>")
endif()

add_subdirectory(${XPLPC_ROOT_DIR}/cxx/lib)

if(XPLPC_ENABLE_INTERFACE)
    message(STATUS "XPLPC Interface: Enabled")
    add_subdirectory(${XPLPC_ROOT_DIR}/c/lib)
endif()

if(XPLPC_TARGET STREQUAL "kotlin")
    add_subdirectory(${XPLPC_ROOT_DIR}/jni/lib)
elseif(XPLPC_TARGET STREQUAL "swift")
    add_subdirectory(${XPLPC_ROOT_DIR}/objc/lib)
    add_subdirectory(${XPLPC_ROOT_DIR}/swift/lib)
    add_subdirectory(${XPLPC_ROOT_DIR}/swift/support)
elseif(XPLPC_TARGET STREQUAL "wasm")
    add_subdirectory(${XPLPC_ROOT_DIR}/wasm/lib)
endif()

if(XPLPC_ADD_CUSTOM_DATA)
    message(STATUS "XPLPC Custom Data: ${XPLPC_CUSTOM_DATA_PATH}")

    # The directory may live outside this tree, which cmake only accepts with a binary directory of its own.
    add_subdirectory(${XPLPC_CUSTOM_DATA_PATH} ${CMAKE_CURRENT_BINARY_DIR}/custom-data)
endif()

if(XPLPC_DEPENDENCY_TOOL STREQUAL "cpm")
    include(${XPLPC_ROOT_DIR}/cmake/cpm/cpm-downloader.cmake)
    include(${XPLPC_ROOT_DIR}/cmake/cpm/cpm-dependency.cmake)
else()
    include(${XPLPC_ROOT_DIR}/cmake/conan/conan-dependency.cmake)
endif()

if(XPLPC_BUILD_SAMPLE)
    if(XPLPC_TARGET STREQUAL "cxx-static")
        add_subdirectory(${XPLPC_ROOT_DIR}/cxx/sample)
    elseif(XPLPC_TARGET MATCHES "^c-(static|shared)$")
        add_subdirectory(${XPLPC_ROOT_DIR}/c/sample)
    endif()
endif()

if(XPLPC_BUILD_TESTS)
    enable_testing()

    if(XPLPC_TARGET STREQUAL "cxx-static")
        add_subdirectory(${XPLPC_ROOT_DIR}/cxx/tests)
    elseif(XPLPC_TARGET MATCHES "^c-(static|shared)$")
        # The suite compiles the public header as c, which is the claim the c abi rests on.
        enable_language(C)
        add_subdirectory(${XPLPC_ROOT_DIR}/c/tests)
    elseif(XPLPC_TARGET STREQUAL "swift")
        add_subdirectory(${XPLPC_ROOT_DIR}/swift/tests)
    endif()
endif()
