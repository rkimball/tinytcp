set( LIB osSupport )
set( SOURCE
        osEvent.cpp
        osEvent.hpp
        osMutex.cpp
        osMutex.hpp
        osPrintfInterface.hpp
        osQueue.cpp
        osQueue.hpp
        osThread.cpp
        osThread.hpp
        osTime.cpp
        osTime.hpp
    )
add_library(${LIB} ${SOURCE})
