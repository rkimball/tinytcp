set( LIB tcpStack )
set( SOURCE
    Address.cpp
    DataBuffer.cpp
    FCS.cpp
    NetworkInterface.cpp
    ProtocolARP.cpp
    ProtocolDHCP.cpp
    ProtocolICMP.cpp
    ProtocolIPv4.cpp
    ProtocolMACEthernet.cpp
    ProtocolTCP.cpp
    ProtocolUDP.cpp
    TCPConnection.cpp
    Utility.cpp
    DefaultStack.cpp
    Config.h
    )
add_library( ${LIB} ${SOURCE} )
add_dependencies( ${LIB} osSupport )
include_directories( ../osSupport )
