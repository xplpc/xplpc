set(XPLPC_INC_FILES ${XPLPC_INC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/JNISupport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/ScopedLocalRef.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/proxy/JNIPlatformProxy.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/ScopedThreadAttachment.hpp"
    PARENT_SCOPE
)

set(XPLPC_SRC_FILES ${XPLPC_SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/jni/JNISupport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/jni/platform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/proxy/JNIPlatformProxy.cpp"
    PARENT_SCOPE
)
