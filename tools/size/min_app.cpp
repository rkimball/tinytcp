// Minimal program that needs the whole stack: used only to measure binary size.
#include "DefaultStack.hpp"

static void Tx(void*, size_t) {}

int main()
{
    static tinytcp::DefaultStack stack;
    uint8_t mac[6] = {0x10, 0xBF, 0x48, 0x44, 0x55, 0x66};
    stack.SetMACAddress(mac);
    stack.RegisterDataTransmitHandler(Tx);
    stack.StartDHCP();
    stack.TCP.NewServer(&stack.MAC, 80);
    static uint8_t rx[64];
    volatile bool run = true;
    while (run)
    {
        stack.ProcessRx(rx, sizeof(rx));
        stack.Tick();
    }
    return 0;
}
