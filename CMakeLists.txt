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

# Embedded C++ constraints for the library and apps: no exceptions, no RTTI, and no thread-safe
# initialization guards for function-local statics (the guard code can throw and drags in the
# whole exception runtime). Not applied to the unit tests, which are built against a separately
# built gtest.
foreach(target os_support tinytcp packet_io web_server test_app)
    target_compile_options(${target} PRIVATE -fno-exceptions -fno-rtti -fno-threadsafe-statics)
endforeach()

add_subdirectory (test)

# ---------------------------------------------------------------------------------------------
# Developer targets: format, tidy, size and size-readme. None of them are part of "all".
# ---------------------------------------------------------------------------------------------
file(GLOB_RECURSE TINYTCP_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
    ${PROJECT_SOURCE_DIR}/test/*.cpp
    ${PROJECT_SOURCE_DIR}/test/*.hpp
    ${PROJECT_SOURCE_DIR}/test_app/*.cpp
    ${PROJECT_SOURCE_DIR}/tools/*.cpp
)
set(TINYTCP_CPP_SOURCES ${TINYTCP_SOURCES})
list(FILTER TINYTCP_CPP_SOURCES INCLUDE REGEX "\\.cpp$")

# make format: reformat every source file in place using .clang-format
find_program(CLANG_FORMAT NAMES clang-format clang-format-18 clang-format-17 clang-format-16
                                clang-format-15 clang-format-14)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${TINYTCP_SOURCES}
        COMMENT "Formatting sources with ${CLANG_FORMAT}"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found, the 'format' target is not available")
endif()

# make tidy: run clang-tidy (checks come from .clang-tidy) on every .cpp file using this build's
# compile_commands.json
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16
                              clang-tidy-15 clang-tidy-14)
if(CLANG_TIDY)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${TINYTCP_CPP_SOURCES}
        COMMENT "Running ${CLANG_TIDY}"
        VERBATIM
    )
else()
    message(STATUS "clang-tidy not found, the 'tidy' target is not available")
endif()

# make size: print the binary size table (markdown) for x86-64, RISC-V and ARM.
# make size-readme: put that table into README.md. See tools/size/measure.sh.
add_custom_target(size
    COMMAND ${PROJECT_SOURCE_DIR}/tools/size/measure.sh
    COMMENT "Measuring binary size (needs the g++, riscv64 and arm cross compilers)"
    USES_TERMINAL
    VERBATIM
)
add_custom_target(size-readme
    COMMAND ${PROJECT_SOURCE_DIR}/tools/size/measure.sh --update-readme
    COMMENT "Updating the size table in README.md"
    USES_TERMINAL
    VERBATIM
)
