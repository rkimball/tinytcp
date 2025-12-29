cmake_minimum_required(VERSION 3.10)

# Include the capability helper module
include(${CMAKE_SOURCE_DIR}/cmake/SetNetRawCapability.cmake)

# Raw Socket Test Application
add_executable(test_raw_socket test_raw_socket.cpp)

# No additional libraries needed for raw sockets on Linux
target_link_libraries(test_raw_socket)

# Automatically set CAP_NET_RAW capability after building
# This allows running tests without sudo
target_add_net_raw_capability(test_raw_socket)

# Installation
install(TARGETS test_raw_socket DESTINATION bin)
