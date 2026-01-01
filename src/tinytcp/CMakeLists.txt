add_library(tinytcp
    data_buffer.cpp
    fcp.cpp
    arp.cpp
    dhcp.cpp
    icmp.cpp
    ipv4.cpp
    mac_ethernet.cpp
    tcp.cpp
    udp.cpp
    tcp_connection.cpp
    utility.cpp
    interface_mac.hpp
    default_stack.cpp
    config.hpp
)
target_include_directories(tinytcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tinytcp os_support)
