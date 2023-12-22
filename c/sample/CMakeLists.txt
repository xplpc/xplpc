# sample
set(XPLPC_SAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_executable(${XPLPC_PROJECT_NAME}-sample "${XPLPC_SAMPLE_FILES}")

target_link_libraries(${XPLPC_PROJECT_NAME}-sample PRIVATE ${XPLPC_PROJECT_NAME})
target_include_directories(${XPLPC_PROJECT_NAME}-sample PRIVATE c/lib/include)
