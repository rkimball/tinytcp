set( LIB tcpStack )
set( SOURCE
    Address.cpp
    DataBuffer.cpp
    FCS.cpp
    NetworkInterface.cpp
    ProtocolARP.cpp
    ProtocolDHCP.cpp
    ProtocolICMP.cpp
    ProtocolIP.cpp
    ProtocolMACEthernet.cpp
    ProtocolTCP.cpp
    ProtocolUDP.cpp
    Utility.cpp
    )
add_library( ${LIB} ${SOURCE} )
add_dependencies( ${LIB} osSupport )
include_directories( ../osSupport )
