# files
set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Anycodable/AnyCodable.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Anycodable/AnyCodableCharacter.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Anycodable/AnyDecodable.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Anycodable/AnyEncodable.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Client/LocalClient.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Client/ProxyClient.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Client/RemoteClient.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Core/XPLPC.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Core/Config.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Data/MappingData.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Map/MappingItem.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Message/Message.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Message/Param.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Message/Request.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Message/Response.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Proxy/PlatformProxy.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Serializer/JsonSerializer.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Serializer/Serializer.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources/Util/Log.swift"
    PARENT_SCOPE
)
