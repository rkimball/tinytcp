set(SOURCE
    DataBuffer.cpp
    FCS.cpp
    ProtocolARP.cpp
    ProtocolDHCP.cpp
    ProtocolICMP.cpp
    ProtocolIPv4.cpp
    ProtocolMACEthernet.cpp
    ProtocolTCP.cpp
    ProtocolUDP.cpp
    TCPConnection.cpp
    Utility.cpp
    InterfaceMAC.hpp
    DefaultStack.cpp
    Config.hpp
    )
add_library(tinytcp ${SOURCE})
target_include_directories(tinytcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tinytcp os_support)
