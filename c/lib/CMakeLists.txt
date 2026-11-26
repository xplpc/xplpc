target_sources(xplpc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/c/platform.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/c/typedefs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/CLayer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/OwnedBytes.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/c/platform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/layer/CLayer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/layer/OwnedBytes.cpp"
)

target_include_directories(xplpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
