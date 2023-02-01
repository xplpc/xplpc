# files
set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/xplpc.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/client/LocalClient.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/client/ProxyClient.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/client/RemoteClient.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/core/XPLPC.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/data/CallbackList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/data/MappingList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/map/Map.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/map/MappingItem.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/message/Message.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/message/Param.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/message/Request.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/message/Response.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/PlatformProxy.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/serializer/Serializer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/type/DataView.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/util/TSMap.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/util/UniqueID.hpp"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/core/XPLPC.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/data/CallbackList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/data/MappingList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/map/MappingItem.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/PlatformProxy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/util/UniqueID.cpp"
    PARENT_SCOPE
)
