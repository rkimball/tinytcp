set( LIB osSupport )
set( SOURCE
    osEvent.cpp
	osEvent.h
    osMutex.cpp
	osMutex.h
    osPrintfInterface.h
    osQueue.cpp
	osQueue.h
    osThread.cpp
	osThread.h
    osTime.cpp
	osTime.h
    )
add_library(${LIB} ${SOURCE})
