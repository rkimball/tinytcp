add_library(os_support
    osEvent.cpp
    osMutex.cpp
    osQueue.cpp
    osThread.cpp
    osTime.cpp
    osUtil.cpp
)
target_include_directories(os_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
