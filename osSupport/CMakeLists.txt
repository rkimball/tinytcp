set( LIB osSupport )
set( SOURCE
    osEvent.cpp
    osMutex.cpp
    osPrintfInterface.h
    osQueue.cpp
    osThread.cpp
    osTime.cpp
    )
add_library(${LIB} ${SOURCE})
