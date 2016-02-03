project(testApp)
cmake_minimum_required(VERSION 2.8)
set( SOURCE
   TestApp.cpp
   PacketIO.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})
target_link_libraries(${PROJECT_NAME} webServer tcpStack osSupport )
add_dependencies( ${PROJECT_NAME} webServer tcpStack osSupport )
include_directories( ../webServer ../tcpStack ../osSupport )
