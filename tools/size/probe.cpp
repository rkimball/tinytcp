// Compiled only so measure.sh can read sizeof(DefaultStack) out of the object file with nm -S.
// That works for cross compilers whose binaries cannot be run on the build machine.
#include "DefaultStack.hpp"

extern const char tinytcp_stack_ram[sizeof(tinytcp::DefaultStack)];
const char tinytcp_stack_ram[sizeof(tinytcp::DefaultStack)] = {1};
