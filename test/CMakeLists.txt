
# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.x
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest SOURCE_DIR BINARY_DIR)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest INTERFACE)
add_dependencies(libgtest gtest)
target_include_directories(libgtest SYSTEM INTERFACE
    ${SOURCE_DIR}/googletest/include
    ${SOURCE_DIR}/googlemock/include)
target_link_libraries(libgtest INTERFACE
    ${BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
)
if(NOT WIN32)
    target_link_libraries(libgtest INTERFACE pthread)
endif()

set (SRC
    main.cpp
    tinytcp/mac.cpp
    tinytcp/test_Utility.cpp
)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(unit-test ${SRC})

target_link_libraries(unit-test pthread libgtest packet_io os_support  tinytcp)

add_dependencies(unit-test libgtest packet_io os_support tinytcp)
# target_include_directories(unit-test PRIVATE ${CMAKE_SOURCE_DIR}/tcp_stack)

add_custom_target(check
    COMMAND ${PROJECT_BINARY_DIR}/test/unit-test
    DEPENDS unit-test
)
