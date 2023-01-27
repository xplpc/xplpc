# files
set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/Mapping.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/Todo.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/AllTypes.hpp"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/custom/PlatformProxy.cpp"
    PARENT_SCOPE
)

if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
        "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/json/TodoJson.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/json/AllTypesJson.hpp"
        PARENT_SCOPE
    )
endif()
