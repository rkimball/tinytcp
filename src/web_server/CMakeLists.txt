add_library(web_server STATIC
    httpd.cpp
    http_page.cpp
)
target_link_libraries(web_server
    os
    tinytcp
)
target_include_directories(web_server PUBLIC
    ../tinytcp)
