cmake_minimum_required (VERSION 3.10)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_DISABLE_SOURCE_CHANGES ON)
set (CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (tinytcp)

option(TINYTCP_DEBUG "Enable verbose debug output (rx header dumps, etc.)" OFF)
if(TINYTCP_DEBUG)
    add_definitions(-DTINYTCP_DEBUG)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Library sources include each other by bare header name
include_directories (
    ${PROJECT_SOURCE_DIR}/src/os
    ${PROJECT_SOURCE_DIR}/src/tinytcp
    ${PROJECT_SOURCE_DIR}/src/packet_io
    ${PROJECT_SOURCE_DIR}/src/web_server
)

add_subdirectory (src/os)
add_subdirectory (src/tinytcp)
add_subdirectory (src/packet_io)
add_subdirectory (src/web_server)
add_subdirectory (test_app)
add_subdirectory (test)

set (CLANG_FORMAT clang-format)
add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/os/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/packet_io/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/tinytcp/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/test/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/test_app/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/web_server/*.?pp
)

set (CLANG_TIDY clang-tidy)
add_custom_target(tidy
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/os/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/packet_io/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/tinytcp/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/test/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/test_app/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/web_server/*.?pp
)
