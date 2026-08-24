# The bootstrap comes from https://github.com/cpm-cmake/CPM.cmake.

set(CPM_DOWNLOAD_VERSION 0.43.1)
set(CPM_DOWNLOAD_HASH 1c40fc102ce9625d7de7eb14f541cab30cc3138dca627f0b0ec40293ce6c2934)

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

# A path carrying a tilde is expanded, since cmake does not do it.
get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")

  # The file is included, so it runs, and a download nobody checks is a script nobody read.
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION}
       EXPECTED_HASH SHA256=${CPM_DOWNLOAD_HASH}
       STATUS CPM_DOWNLOAD_STATUS
  )

  list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_RESULT)

  if(NOT CPM_DOWNLOAD_RESULT EQUAL 0)
    list(GET CPM_DOWNLOAD_STATUS 1 CPM_DOWNLOAD_ERROR)
    file(REMOVE ${CPM_DOWNLOAD_LOCATION})
    message(FATAL_ERROR "Failed to download CPM.cmake: ${CPM_DOWNLOAD_ERROR}")
  endif()
endif()

include(${CPM_DOWNLOAD_LOCATION})
