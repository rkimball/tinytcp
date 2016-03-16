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


foreach( file ${HTML} )
    configure_file( ${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} @ONLY )
endforeach( file )
#file( COPY ${HTML} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( "-DBINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/\"" )


link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/WinPcap/Lib )
add_executable(${PROJECT_NAME} ${SOURCE} )

target_link_libraries(${PROJECT_NAME} ${LIBS} )
add_dependencies( ${PROJECT_NAME} ${LIBS} ${HTML} )
include_directories( ../webServer ../tcpStack ../osSupport )
if( WIN32 )
include_directories( WinPcap/include )
endif( WIN32 )
