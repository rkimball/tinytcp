# SetNetRawCapability.cmake
# CMake module to grant CAP_NET_RAW capability to executables
# This allows raw socket access without requiring sudo to run the tests

# Function to add CAP_NET_RAW capability to a target after it's built
# Usage: target_add_net_raw_capability(target_name)
function(target_add_net_raw_capability TARGET_NAME)
    if(UNIX AND NOT APPLE)
        # Add a post-build command to set CAP_NET_RAW on the executable
        add_custom_command(TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND sudo -n /usr/sbin/setcap cap_net_raw,cap_net_admin+eip $<TARGET_FILE:${TARGET_NAME}> || 
                    echo "Warning: Could not set capabilities on ${TARGET_NAME}. You may need to run with sudo."
            COMMENT "Setting CAP_NET_RAW capability on ${TARGET_NAME}"
            VERBATIM
        )
    endif()
endfunction()

# Alternative: Create a custom target to set capabilities on all raw socket tests
# Usage: add_setcap_target(setcap_tests test1 test2 test3)
function(add_setcap_target TARGET_NAME)
    if(UNIX AND NOT APPLE)
        set(SETCAP_COMMANDS "")
        foreach(TEST_TARGET ${ARGN})
            list(APPEND SETCAP_COMMANDS 
                COMMAND sudo -n /usr/sbin/setcap cap_net_raw,cap_net_admin+eip $<TARGET_FILE:${TEST_TARGET}>
            )
        endforeach()
        
        add_custom_target(${TARGET_NAME}
            ${SETCAP_COMMANDS}
            COMMENT "Setting CAP_NET_RAW capability on raw socket test executables"
            VERBATIM
        )
        
        # Make the setcap target depend on all test targets
        foreach(TEST_TARGET ${ARGN})
            add_dependencies(${TARGET_NAME} ${TEST_TARGET})
        endforeach()
    endif()
endfunction()
