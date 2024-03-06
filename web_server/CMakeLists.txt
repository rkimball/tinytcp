set(SOURCE
    httpd.cpp
    http_page.cpp
)
add_library(web_server ${SOURCE})
target_link_libraries(web_server
    os_support
    tinytcp
)
