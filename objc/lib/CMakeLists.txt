# files
set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/objc/support.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/XPLPCNativePlatformProxy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/XPLPCNativePlatformProxyImpl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/XPLPCNativePlatformProxyManager.h"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/objc/support.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/XPLPCNativePlatformProxy.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/XPLPCNativePlatformProxyImpl.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/XPLPCNativePlatformProxyManager.mm"
    PARENT_SCOPE
)

set(XPLPC_PUBLIC_HEADER_FILES ${XPLPC_PUBLIC_HEADER_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/XPLPCNativePlatformProxyImpl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/XPLPCNativePlatformProxyManager.h"
    PARENT_SCOPE
)
