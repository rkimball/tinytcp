project(test_app)
cmake_minimum_required(VERSION 3.10)
set( SOURCE
   TestApp.cpp
   )
set( HTML
    master.html
    )
set( LIBS
   web_server
   tinytcp
   os_support
   packet_io )

foreach( file ${HTML} )
    configure_file( ${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} @ONLY )
endforeach( file )

add_definitions( "-DBINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/\"" )

add_executable(${PROJECT_NAME} ${SOURCE} )

target_link_libraries(${PROJECT_NAME} ${LIBS} pthread)
add_dependencies( ${PROJECT_NAME} ${LIBS} packet_io )