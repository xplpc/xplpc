target_sources(xplpc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/JNIEnvironment.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/JNISupport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/ScopedGlobalRef.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/ScopedLocalRef.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/jni/ScopedThreadAttachment.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xplpc/layer/JNILayer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/jni/JNIEnvironment.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/jni/JNISupport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/jni/platform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xplpc/layer/JNILayer.cpp"
)

target_include_directories(xplpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ANDROID)
    find_library(ANDROID_LOG_LIB log)
    target_link_libraries(xplpc PUBLIC ${ANDROID_LOG_LIB})

    # The ndk links its c++ runtime statically, and a library exports what it links unless told not to, so only the jni entry points stay visible.
    target_link_options(xplpc PRIVATE "-Wl,--exclude-libs,ALL")
else()
    if(NOT DEFINED ENV{JAVA_HOME})
        message(FATAL_ERROR "JAVA_HOME has to point at a jdk, since the jni headers come from there")
    endif()

    target_include_directories(xplpc PRIVATE $ENV{JAVA_HOME}/include)

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_include_directories(xplpc PRIVATE $ENV{JAVA_HOME}/include/win32)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_include_directories(xplpc PRIVATE $ENV{JAVA_HOME}/include/darwin)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_include_directories(xplpc PRIVATE $ENV{JAVA_HOME}/include/linux)
    endif()
endif()
