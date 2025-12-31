add_library(packet_io packet_io.cpp)
target_include_directories(packet_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(packet_io PUBLIC tinytcp)
