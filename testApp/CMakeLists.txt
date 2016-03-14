project(testApp)
cmake_minimum_required(VERSION 2.8)
set( SOURCE
   TestApp.cpp
   PacketIO.h
   PacketIO.cpp
   )
set( HTML
    master.html
    )
set( LIBS
   webServer
   tcpStack
   osSupport )
if( UNIX )
	set( LIBS ${LIBS} pthread )
elseif(WIN32)
	set( LIBS ${LIBS} Packet.lib wpcap.lib Ws2_32.lib )
endif()
file( COPY ${HTML} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/WinPcap/Lib )
add_executable(${PROJECT_NAME} ${SOURCE} ${HTML})



target_link_libraries(${PROJECT_NAME} ${LIBS} )
add_dependencies( ${PROJECT_NAME} webServer tcpStack osSupport )
include_directories( ../webServer ../tcpStack ../osSupport )
if( WIN32 )
include_directories( WinPcap/include )
endif( WIN32 )
