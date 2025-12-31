add_library(web_server
    httpd.cpp
    http_page.cpp
)
target_include_directories(web_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(web_server
    os_support
    tinytcp
)
