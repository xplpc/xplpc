target_sources(xplpc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/ObjCLayer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/ObjCLayerBridge.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/ObjCLayerHost.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/objc/support.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/layer/ObjCLayer.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/layer/ObjCLayerBridge.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/objc/support.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/platform/ObjCPlatformInitializer.mm"
)

target_include_directories(xplpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Only the objective-c bridge uses Foundation, so the libraries the other hosts load do not bring the objective-c runtime with them.
find_library(FWK_FOUNDATION Foundation REQUIRED)
target_link_libraries(xplpc PRIVATE ${FWK_FOUNDATION})

# Swift implements the host protocol and calls the bridge, so these are the only headers of this layer the framework ships.
set_property(TARGET xplpc APPEND PROPERTY PUBLIC_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/ObjCLayerBridge.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/ObjCLayerHost.h"
)
