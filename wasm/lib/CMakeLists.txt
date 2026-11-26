target_sources(xplpc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/bind/JavascriptBridge.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/bind/JavascriptLayer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/bind/Bind.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/bind/JavascriptBridge.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/bind/JavascriptLayer.cpp"
)

target_link_libraries(xplpc PRIVATE embind)

target_link_options(xplpc PRIVATE
    -fwasm-exceptions
    -sMALLOC=emmalloc
    -sALLOW_MEMORY_GROWTH
    -sEXPORT_ES6
    -sEXPORTED_FUNCTIONS=_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=HEAPU8
    $<$<CONFIG:Debug>:-sASSERTIONS>
)
