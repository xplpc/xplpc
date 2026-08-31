cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "xplpc")
project(${PROJECT_NAME} LANGUAGES CXX)

# Flutter reaches this through a symlink, so the repository is found by resolving it rather than by counting segments.
get_filename_component(XPLPC_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(XPLPC_ROOT_DIR "${XPLPC_PLUGIN_DIR}/../../.." ABSOLUTE)

file(GLOB XPLPC_NATIVE_LIBRARY "${XPLPC_ROOT_DIR}/build/c-shared/*/lib/libxplpc.so")

if(NOT XPLPC_NATIVE_LIBRARY)
    message(FATAL_ERROR "The native library was not found, build it with: python3 xplpc.py c-build-shared")
endif()

set(xplpc_bundled_libraries
    "${XPLPC_NATIVE_LIBRARY}"
    PARENT_SCOPE
)
