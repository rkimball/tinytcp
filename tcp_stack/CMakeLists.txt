set( LIB tcpStack )
set( SOURCE
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
add_library( ${LIB} ${SOURCE} )
add_dependencies( ${LIB} osSupport )
include_directories( ../osSupport )
