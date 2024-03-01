add_library(tinytcp
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

# add dependencies for os
target_include_directories(tinytcp PUBLIC ../os)

# target_link_directories(tinytcp PUBLIC os)
add_dependencies(tinytcp os)
