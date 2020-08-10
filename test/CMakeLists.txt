
# Enable ExternalProject CMake module
include(ExternalProject)

# message(STATUS "*********** CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
set(SAVED_CXX_FLAGS ${CMAKE_CXX_FLAGS})
# string(REPLACE "/W3" "/w" CMAKE_CXX_FLAGS, ${CMAKE_CXX_FLAGS})
# message(STATUS "*********** new CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /GR /EHsc /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
# message(STATUS "*********** other CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")


# Enable ExternalProject CMake module
include(ExternalProject)

#------------------------------------------------------------------------------
# Download and install GoogleTest ...
#------------------------------------------------------------------------------

SET(GTEST_GIT_REPO_URL https://github.com/google/googletest.git)
SET(GTEST_GIT_LABEL release-1.10.0)

set(GMOCK_OUTPUT_DIR ${EXTERNAL_PROJECTS_ROOT}/gtest/build/lib)
set(GTEST_OUTPUT_DIR ${GMOCK_OUTPUT_DIR})

if(WIN32)
    list(APPEND GTEST_CMAKE_ARGS
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=${GTEST_OUTPUT_DIR}
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=${GTEST_OUTPUT_DIR}
        -Dgtest_force_shared_crt=TRUE
    )
    set(GMOCK_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
endif()

if(CMAKE_BUILD_TYPE)
    list(APPEND GTEST_CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
endif()

if(UNIX)
    # workaround for compile error
    # related: https://github.com/intel/mkl-dnn/issues/55
    set(GTEST_CXX_FLAGS "-Wno-unused-result ${CMAKE_ORIGINAL_CXX_FLAGS} -Wno-undef")
else()
    set(GTEST_CXX_FLAGS ${CMAKE_ORIGINAL_CXX_FLAGS})
endif()

#Build for ninja
SET(GTEST_PATHS ${GTEST_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtestd${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${GMOCK_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmockd${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${GTEST_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${GMOCK_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})

ExternalProject_Add(
    gtest
    PREFIX gtest
    GIT_REPOSITORY ${GTEST_GIT_REPO_URL}
    GIT_TAG ${GTEST_GIT_LABEL}
    # Disable install step
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    CMAKE_GENERATOR ${CMAKE_GENERATOR}
    CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}
    CMAKE_GENERATOR_TOOLSET ${CMAKE_GENERATOR_TOOLSET}
    CMAKE_ARGS /DWIN32 /D_WINDOWS /GR /EHsc /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    BINARY_DIR "${EXTERNAL_PROJECTS_ROOT}/gtest/build"
    EXCLUDE_FROM_ALL TRUE
    BUILD_BYPRODUCTS ${GTEST_PATHS}
)

#------------------------------------------------------------------------------

ExternalProject_Get_Property(gtest SOURCE_DIR BINARY_DIR)

add_library(libgtest INTERFACE)
add_dependencies(libgtest gtest)
target_include_directories(libgtest SYSTEM INTERFACE
    ${SOURCE_DIR}/googletest/include
    ${SOURCE_DIR}/googlemock/include)
target_compile_definitions(libgtest INTERFACE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)

if(LINUX OR APPLE OR WIN32)
    target_link_libraries(libgtest INTERFACE
        debug ${GTEST_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtestd${CMAKE_STATIC_LIBRARY_SUFFIX}
        debug ${GMOCK_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmockd${CMAKE_STATIC_LIBRARY_SUFFIX}
        optimized ${GTEST_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
        optimized ${GMOCK_OUTPUT_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})
    if(NOT WIN32)
        target_link_libraries(libgtest INTERFACE pthread)
    endif()
else()
    message(FATAL_ERROR "libgtest: Unsupported platform.")
endif()









# # Download and install GoogleTest
# ExternalProject_Add(
#     gtest
#     GIT_REPOSITORY https://github.com/google/googletest.git
#     GIT_TAG     release-1.10.0
#     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
#     # Disable install step
#     INSTALL_COMMAND ""
#     UPDATE_COMMAND ""
# )

# # Get GTest source and binary directories from CMake project
# ExternalProject_Get_Property(gtest SOURCE_DIR BINARY_DIR)

# # Create a libgtest target to be used as a dependency by test programs
# add_library(libgtest INTERFACE)
# add_dependencies(libgtest gtest)
# target_include_directories(libgtest SYSTEM INTERFACE
#     ${SOURCE_DIR}/googletest/include
#     ${SOURCE_DIR}/googlemock/include)
# target_link_libraries(libgtest INTERFACE
#     ${BINARY_DIR}/googlemock/gtest//${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
# )
# target_compile_definitions(libgtest INTERFACE -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
# if(NOT WIN32)
#     target_link_libraries(libgtest INTERFACE pthread)
# endif()

set (SRC
    main.cpp
    network.cpp
    util.cpp
)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(unit-test ${SRC})

target_link_libraries(unit-test pthread libgtest packet_io osSupport)

add_dependencies(unit-test libgtest packet_io osSupport)

add_custom_target(check
COMMAND ${PROJECT_BINARY_DIR}/test/unit-test
DEPENDS unit-test)

set(CMAKE_CXX_FLAGS ${SAVED_CXX_FLAGS})
