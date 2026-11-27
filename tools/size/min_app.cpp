// Minimal program that needs the whole stack: used only to measure binary size.
//
// The stack is a global, like in test_app. A function-local static would make the compiler emit
// a thread-safe initialization guard (__cxa_guard_acquire), which pulls the whole C++ exception
// runtime into the binary and would hide the real size of the stack.
#include "DefaultStack.hpp"

static void Tx(void*, size_t) {}

static tinytcp::DefaultStack stack;
static uint8_t rx[64];

int main()
{
    uint8_t mac[6] = {0x10, 0xBF, 0x48, 0x44, 0x55, 0x66};
    stack.SetMACAddress(mac);
    stack.RegisterDataTransmitHandler(Tx);
    stack.StartDHCP();
    stack.TCP.NewServer(&stack.MAC, 80);
    volatile bool run = true;
    while (run)
    {
        stack.ProcessRx(rx, sizeof(rx));
        stack.Tick();
    }
    return 0;
}
