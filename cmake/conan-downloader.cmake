# ref: https://github.com/conan-io/cmake-conan

set(CONAN_DOWNLOAD_VERSION 0.18.1)
set(CONAN_DOWNLOAD_LOCATION "${CMAKE_SOURCE_DIR}/build/conan/conan.cmake")

if(NOT EXISTS ${CONAN_DOWNLOAD_LOCATION})
    message(STATUS "Downloading conan.cmake to ${CONAN_DOWNLOAD_LOCATION}")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/${CONAN_DOWNLOAD_VERSION}/conan.cmake" "${CONAN_DOWNLOAD_LOCATION}")
endif()

include(${CONAN_DOWNLOAD_LOCATION})
