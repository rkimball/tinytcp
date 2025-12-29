#!/bin/bash
#------------------------------------------------------------------------------
# Script to set CAP_NET_RAW capability on all test executables
# Run this after building if the automatic post-build step failed
#------------------------------------------------------------------------------

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# List of executables that need raw socket capability
EXECUTABLES=(
    "test/raw_sockets/test_raw_socket"
    "test_app/test_app"
)

echo "Setting CAP_NET_RAW capability on test executables..."
echo "Build directory: ${BUILD_DIR}"
echo

for exe in "${EXECUTABLES[@]}"; do
    exe_path="${BUILD_DIR}/${exe}"
    if [[ -f "${exe_path}" ]]; then
        echo "  Setting capability on: ${exe}"
        sudo setcap cap_net_raw,cap_net_admin+eip "${exe_path}"
        # Verify
        getcap "${exe_path}"
    else
        echo "  Skipping (not found): ${exe}"
    fi
done

echo
echo "Done! You can now run the tests without sudo."
echo "Example:"
echo "  cd build/test/raw_sockets && ./test_raw_socket"
