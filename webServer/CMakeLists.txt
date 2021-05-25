set( LIB webServer )
set( SOURCE
    HTTPD.cpp
    HTTPPage.cpp
    )
add_library(${LIB} ${SOURCE})
include_directories( ../tcpStack ../os_support )
