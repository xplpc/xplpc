# files
set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/c/platform.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/c/typedefs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/CNativePlatformProxy.hpp"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/c/platform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/CNativePlatformProxy.cpp"
    PARENT_SCOPE
)
