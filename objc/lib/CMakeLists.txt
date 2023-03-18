# files
set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/objc/support.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/ObjCPlatformProxy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/ObjCPlatformProxyImpl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/ObjCPlatformProxyManager.h"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/objc/support.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/ObjCPlatformProxy.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/ObjCPlatformProxyImpl.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/ObjCPlatformProxyManager.mm"
    PARENT_SCOPE
)

set(XPLPC_PUBLIC_HEADER_FILES ${XPLPC_PUBLIC_HEADER_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/ObjCPlatformProxyImpl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/ObjCPlatformProxyManager.h"
    PARENT_SCOPE
)
