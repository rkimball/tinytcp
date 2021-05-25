cmake_minimum_required (VERSION 2.8)
set (CMAKE_DISABLE_SOURCE_CHANGES ON)
set (CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (tinytcp)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=inconsistent-missing-override")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=non-virtual-dtor")

     # whitelist errors here
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat-pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-global-constructors")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-newline-eof")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undef")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-exit-time-destructors")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-variable-declarations")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-documentation")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-covered-switch-default")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")

    # # should remove these
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-old-style-cast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-float-conversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-conversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-padded")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-potentially-evaluated-expression") # Triggers false alarms on typeid
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables") # Not ready for this yet
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-float-equal")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-duplicate-enum") # from numpy
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-used-but-marked-unused") # from sox
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-compat-deprecated-writable-strings")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-double-promotion")
endif()

add_subdirectory (osSupport)
add_subdirectory (tcp_stack)
include_directories(tcp_stack osSupport)
add_subdirectory (webServer)
add_subdirectory (packet_io)
include_directories (packet_io)
add_subdirectory (testApp)
add_subdirectory (test)

set (CLANG_FORMAT clang-format-10)
add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/osSupport/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/packet_io/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/tcp_stack/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/test/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/testApp/*.?pp
    COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/webServer/*.?pp
)
