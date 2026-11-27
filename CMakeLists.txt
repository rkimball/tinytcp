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

# Code coverage. Configure with -DTINYTCP_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug, build, and run
# "make coverage". Use Debug, not Release: optimization makes the line numbers meaningless.
# The minimums are percentages and "make coverage" fails if line coverage is below them. They are
# set a little under what the tests reach today. Raise them as tests are added.
option(TINYTCP_COVERAGE "Instrument the code with gcov and add the coverage target" OFF)
set(TINYTCP_COVERAGE_MIN_LINES 52 CACHE STRING "Minimum line coverage of src/, in percent")
set(TINYTCP_COVERAGE_MIN_LINES_OS 99 CACHE STRING "Minimum line coverage of src/os, in percent")
if(TINYTCP_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    if(CMAKE_BUILD_TYPE MATCHES "Rel")
        message(WARNING "TINYTCP_COVERAGE with a Release build gives misleading coverage, use Debug")
    endif()
endif()

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

# make coverage: run the unit tests, write a report to <build>/coverage (index.html and
# summary.txt) and fail if line coverage is below TINYTCP_COVERAGE_MIN_LINES for src/ or
# TINYTCP_COVERAGE_MIN_LINES_OS for src/os. Needs -DTINYTCP_COVERAGE=ON and gcovr.
if(TINYTCP_COVERAGE)
    find_program(GCOVR gcovr)
    if(GCOVR)
        set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
        set(GCOVR_COMMON ${GCOVR} --root ${PROJECT_SOURCE_DIR} --object-directory ${CMAKE_BINARY_DIR})
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
            # Counters left over from an earlier run would be added to this one
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
            COMMAND $<TARGET_FILE:unit-test>
            COMMAND ${GCOVR_COMMON} --filter ${PROJECT_SOURCE_DIR}/src/
                    --txt ${COVERAGE_DIR}/summary.txt --html-details ${COVERAGE_DIR}/index.html
            COMMAND ${CMAKE_COMMAND} -E cat ${COVERAGE_DIR}/summary.txt
            COMMAND ${GCOVR_COMMON} --filter ${PROJECT_SOURCE_DIR}/src/ --output ${COVERAGE_DIR}/check-all.txt
                    --print-summary --fail-under-line ${TINYTCP_COVERAGE_MIN_LINES}
            COMMAND ${GCOVR_COMMON} --filter ${PROJECT_SOURCE_DIR}/src/os/ --output ${COVERAGE_DIR}/check-os.txt
                    --print-summary --fail-under-line ${TINYTCP_COVERAGE_MIN_LINES_OS}
            DEPENDS unit-test
            COMMENT "Running the unit tests and checking coverage"
            USES_TERMINAL
            VERBATIM
        )
    else()
        message(STATUS "gcovr not found, the 'coverage' target is not available")
    endif()
endif()
