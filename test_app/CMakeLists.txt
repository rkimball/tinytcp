project(test_app)
cmake_minimum_required(VERSION 3.10)

# Include the capability helper module for raw socket support
include(${CMAKE_SOURCE_DIR}/cmake/SetNetRawCapability.cmake)

set(HTML
    master.html
)

foreach(file ${HTML})
    configure_file(${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} @ONLY)
endforeach(file)
#file(COPY ${HTML} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_definitions("-DBINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/\"")

add_executable(test_app TestApp.cpp)

target_link_libraries(test_app 
    web_server
    tinytcp
    os_support
    packet_io
    pthread
)


# Automatically set CAP_NET_RAW capability after building (Linux only)
# This allows running the test app without sudo in the dev container
target_add_net_raw_capability(test_app)
