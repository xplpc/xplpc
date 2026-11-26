cmake_minimum_required(VERSION 3.25)
project(xplpc-consumer LANGUAGES CXX)

include(FetchContent)

# This file follows main so it always builds, and a project copying it replaces main with the main-<commit> release it was written against.
# Setting FETCHCONTENT_SOURCE_DIR_XPLPC points it at a local checkout instead of fetching anything.
FetchContent_Declare(
    xplpc
    GIT_REPOSITORY https://github.com/xplpc/xplpc.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

set(XPLPC_TARGET "cxx-static" CACHE STRING "What the library is built for")
FetchContent_MakeAvailable(xplpc)

# The mappings are compiled into the library itself, which is what lets the same lines serve a shared target that has no executable to carry them.
target_sources(xplpc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/consumer/Greeting.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NativeLayer.cpp
)

target_include_directories(xplpc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(xplpc-consumer ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(xplpc-consumer PRIVATE xplpc::xplpc)
