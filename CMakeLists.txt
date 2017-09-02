cmake_minimum_required (VERSION 2.8)
set (CMAKE_DISABLE_SOURCE_CHANGES ON)
set (CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if (POLICY CMP0046)
    cmake_policy(SET CMP0046 OLD)
endif()

project (tinytcp)

add_subdirectory (osSupport)
add_subdirectory (tcpStack)
include_directories(tcpStack osSupport)
add_subdirectory (webServer)
add_subdirectory (packet_io)
include_directories (packet_io)
add_subdirectory (testApp)
add_subdirectory (test)

