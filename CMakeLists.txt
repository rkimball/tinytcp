cmake_minimum_required(VERSION 2.8)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(tinytcp)

add_subdirectory(osSupport)
add_subdirectory(tcpStack)
add_subdirectory(webServer)
add_subdirectory(testApp)
add_subdirectory(test)

