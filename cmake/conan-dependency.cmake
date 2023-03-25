list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(CONAN_REQUIRES "")
list(APPEND CONAN_REQUIRES "spdlog/1.11.0")
list(APPEND CONAN_REQUIRES "nlohmann_json/3.11.2")

if(XPLPC_ENABLE_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        list(APPEND CONAN_REQUIRES "gtest/1.13.0")
    endif()
endif()

set(CONAN_IMPORTS
    "bin, *.dll -> ./bin"
    "lib, *.dylib -> ./lib"
    "lib, *.so -> ./lib"
    "bin, *.pdb -> ./bin"
    "lib, *.dll -> ./bin"
    "plugins, * -> ./bin"
)

conan_cmake_configure(
    REQUIRES ${CONAN_REQUIRES}
    GENERATORS CMakeDeps
    IMPORTS ${CONAN_IMPORTS}
)

conan_cmake_autodetect(settings)

conan_cmake_install(
    PATH_OR_REFERENCE .
    BUILD missing
    REMOTE conancenter
    SETTINGS ${settings}
)

find_package(fmt REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

if(XPLPC_ENABLE_SERIALIZER_FOR_JSON)
    find_package(nlohmann_json REQUIRED CONFIG)
endif()

if(XPLPC_ENABLE_TESTS)
    if("${XPLPC_TARGET}" MATCHES "^(cxx|c)-(static|shared)$")
        find_package(gtest REQUIRED CONFIG)
    endif()
endif()
