#include "InterfaceMAC.hpp"

class LocalMAC: public InterfaceMAC {
public:
    typedef void (*TxHandler)(const uint8_t* src, const uint8_t* data, size_t len);

    LocalMAC(TxHandler, const uint8_t* mac = 0);
    ~LocalMAC();
    void Initialize();
    void Send(const uint8_t* dest, const uint8_t* data, size_t len);

private:
    TxHandler RxCallback;
    uint8_t MAC[6];
};
