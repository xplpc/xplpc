if(XPLPC_TARGET STREQUAL "kotlin")
    if (CMAKE_SYSTEM_NAME MATCHES "Android")
        # android
        find_library(ANDROID_LIB android)
        find_library(ANDROID_LOG_LIB log)
        target_link_libraries(${XPLPC_PROJECT_NAME} PUBLIC ${ANDROID_LIB} ${ANDROID_LOG_LIB})
    else()
        # desktop
        target_include_directories(${XPLPC_PROJECT_NAME} PRIVATE $ENV{JAVA_HOME}/include)

        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            target_include_directories(${XPLPC_PROJECT_NAME} PRIVATE $ENV{JAVA_HOME}/include/win32)
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            target_include_directories(${XPLPC_PROJECT_NAME} PRIVATE $ENV{JAVA_HOME}/include/darwin)
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_include_directories(${XPLPC_PROJECT_NAME} PRIVATE $ENV{JAVA_HOME}/include/linux)
        endif()
    endif()
endif()
