target_sources(xplpc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/AllTypes.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/Mapping.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/Todo.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/json/AllTypesJson.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/json/JsonField.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/custom/json/TodoJson.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/custom/NativeLayer.cpp"
)

target_include_directories(xplpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
