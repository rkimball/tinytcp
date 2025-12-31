cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(tinytcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_subdirectory(src)
add_subdirectory(packet_io)
add_subdirectory(web_server)
add_subdirectory(test_app)
add_subdirectory(test)

set(CLANG_FORMAT clang-format)
add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/os_support/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/src/tcp_stack/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/test/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/test_app/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/web_server/*.?pp
)

set(CLANG_TIDY clang-tidy)
add_custom_target(tidy
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/os_support/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/src/tcp_stack/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/test/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/test_app/*.?pp
    COMMAND ${CLANG_TIDY} -p ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/web_server/*.?pp
)
