//----------------------------------------------------------------------------
// Copyright( c ) 2016, Robert Kimball
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#include <cstring>

#include "TCPConnection.hpp"
#include "ProtocolIPv4.hpp"
#include "ProtocolTCP.hpp"
#include "Utility.hpp"
#include "osTime.hpp"

//============================================================================
//
//============================================================================

TCPConnection::TCPConnection()
    : RxInOffset(0)
    , RxOutOffset(0)
    , TxOffset(0)
    , CurrentWindow(TCP_RX_WINDOW_SIZE)
    , TxBuffer(0)
    , RxBufferEmpty(true)
    , NewConnection(0)
    , Parent(0)
    , Event("tcp connection")
    , HoldingQueue("TCPHolding", TX_BUFFER_COUNT, ConnectionHoldingBuffer)
    , HoldingQueueLock("HoldingQueueLock")
    , MAC(0)
    , IP(0)
    , TCP(0)
{
}

//============================================================================
//
//============================================================================

void TCPConnection::Initialize(ProtocolIPv4& ip, ProtocolTCP& tcp)
{
    MAC = 0;
    IP  = &ip;
    TCP = &tcp;
}

//============================================================================
//
//============================================================================

TCPConnection::~TCPConnection()
{
}

//============================================================================
//
//============================================================================

void TCPConnection::SetMAC(InterfaceMAC* mac)
{
    MAC = mac;
}

//============================================================================
//
//============================================================================

void TCPConnection::SendFlags(uint8_t flags)
{
    DataBuffer* buffer = GetTxBuffer();

    if (buffer)
    {
        BuildPacket(buffer, flags);
    }
}

//============================================================================
//
//============================================================================

void TCPConnection::BuildPacket(DataBuffer* buffer, uint8_t flags)
{
    uint8_t* packet;
    uint16_t checksum;
    uint16_t length;

    flags |= FLAG_ACK;

    buffer->Packet -= ProtocolTCP::header_size();
    packet = buffer->Packet;
    length = buffer->Length;
    if (packet != 0)
    {
        Pack16(packet, 0, LocalPort);
        Pack16(packet, 2, RemotePort);
        Pack32(packet, 4, SequenceNumber);
        if ((int32_t)(AcknowledgementNumber - LastAck) > 0)
        {
            // Only advance LastAck if Ack > LastAck
            LastAck = AcknowledgementNumber;
        }
        Pack32(packet, 8, AcknowledgementNumber);
        packet[12] = 0x50; // Header length and reserved
        packet[13] = flags;
        Pack16(packet, 14, CurrentWindow);
        Pack16(packet, 16, 0); // checksum placeholder
        Pack16(packet, 18, 0); // urgent pointer

        SequenceNumber += length;
        buffer->AcknowledgementNumber = SequenceNumber;

        while ((int32_t)(MaxSequenceTx - SequenceNumber) < 0)
        {
            printf("tx window full\n");
            Event.Wait(__FILE__, __LINE__);
        }

        checksum = ProtocolTCP::ComputeChecksum(
            packet, length + ProtocolTCP::header_size(), IP->GetUnicastAddress(), RemoteAddress);

        Pack16(packet, 16, checksum); // checksum

        buffer->Length += ProtocolTCP::header_size();

        if (length > 0 || (flags & (FLAG_SYN | FLAG_FIN)))
        {
            buffer->Disposable = false;
            buffer->Time_us    = (uint32_t)osTime::GetTime();
            HoldingQueueLock.Take(__FILE__, __LINE__);
            HoldingQueue.Put(buffer);
            HoldingQueueLock.Give();
        }

        IP->Transmit(buffer, 0x06, RemoteAddress, IP->GetUnicastAddress());
    }
}

//============================================================================
//
//============================================================================

DataBuffer* TCPConnection::GetTxBuffer()
{
    DataBuffer* rc;

    rc = IP->GetTxBuffer(MAC);
    if (rc)
    {
        rc->Packet += ProtocolTCP::header_size();
        rc->Remainder -= ProtocolTCP::header_size();
    }

    return rc;
}

//============================================================================
//
//============================================================================

void TCPConnection::Write(const uint8_t* data, uint16_t length)
{
    uint16_t i;

    while (length > 0)
    {
        if (!TxBuffer)
        {
            TxBuffer = GetTxBuffer();
            TxOffset = 0;
        }

        if (TxBuffer)
        {
            if (TxBuffer->Remainder > length)
            {
                for (i = 0; i < length; i++)
                {
                    TxBuffer->Packet[TxOffset++] = data[i];
                }
                TxBuffer->Length += length;
                TxBuffer->Remainder -= length;
                break;
            }
            else if (TxBuffer->Remainder <= length)
            {
                for (i = 0; i < TxBuffer->Remainder; i++)
                {
                    TxBuffer->Packet[TxOffset++] = data[i];
                }
                TxBuffer->Length += TxBuffer->Remainder;
                length -= TxBuffer->Remainder;
                data += TxBuffer->Remainder;
                TxBuffer->Remainder = 0;
                Flush();
            }
        }
        else
        {
            printf("Out of tx buffers\n");
        }
    }
}

//============================================================================
//
//============================================================================

void TCPConnection::Flush()
{
    if (TxBuffer != 0)
    {
        BuildPacket(TxBuffer, FLAG_PSH);
        TxBuffer = 0;
    }
}

//============================================================================
//
//============================================================================

void TCPConnection::Close()
{
    switch (State)
    {
    case LISTEN:
    case SYN_SENT: State = CLOSED; break;
    case SYN_RECEIVED:
    case ESTABLISHED:
        SendFlags(FLAG_FIN);
        State = FIN_WAIT_1;
        SequenceNumber++; // FIN consumes a sequence number
        break;
    case CLOSE_WAIT:
        SendFlags(FLAG_FIN);
        SequenceNumber++; // FIN consumes a sequence number
        State = LAST_ACK;
        break;
    case CLOSED:
        break;
    case FIN_WAIT_1:
        break;
    case FIN_WAIT_2:
        break;
    case CLOSING:
        break;
    case LAST_ACK:
        break;
    case TIMED_WAIT:
        break;
    case TTCP_PERSIST:
        break;
    }
}

//============================================================================
//
//============================================================================

TCPConnection* TCPConnection::Listen()
{
    TCPConnection* connection;

    while (NewConnection == 0)
    {
        Event.Wait(__FILE__, __LINE__);
    }
    connection    = NewConnection;
    NewConnection = 0;

    return connection;
}

//============================================================================
//
//============================================================================

int TCPConnection::Read()
{
    int rc = -1;

    while (RxBufferEmpty)
    {
        if (LastAck != AcknowledgementNumber)
        {
            SendFlags(FLAG_ACK);
        }
        Event.Wait(__FILE__, __LINE__);
    }

    rc = RxBuffer[RxOutOffset++];
    if (RxOutOffset >= TCP_RX_WINDOW_SIZE)
    {
        RxOutOffset = 0;
    }
    AcknowledgementNumber++;
    CurrentWindow++;

    if (RxOutOffset == RxInOffset)
    {
        RxBufferEmpty = true;
    }

    //if( CurrentWindow == TCP_RX_WINDOW_SIZE && LastAck != AcknowledgementNumber )
    //{
    //   // The Rx buffer is empty, might as well ack
    //   Send( FLAG_ACK );
    //}

    return rc;
}

//============================================================================
//
//============================================================================

int TCPConnection::Read(char* buffer, int size)
{
    int bytes_to_read = std::min(size, TCP_RX_WINDOW_SIZE - RxOutOffset);

    memcpy(buffer, &RxBuffer[RxOutOffset], bytes_to_read);

    RxOutOffset += bytes_to_read;
    if (RxOutOffset >= TCP_RX_WINDOW_SIZE)
    {
        RxOutOffset = 0;
    }
    AcknowledgementNumber += bytes_to_read;
    CurrentWindow += bytes_to_read;

    if (RxOutOffset == RxInOffset)
    {
        RxBufferEmpty = true;
    }

    return bytes_to_read;
}

//============================================================================
//
//============================================================================

int TCPConnection::ReadLine(char* buffer, int size)
{
    int  i;
    char c;
    bool done           = false;
    int  bytesProcessed = 0;

    while (!done)
    {
        i = Read();
        if (i == -1)
        {
            *buffer = 0;
            break;
        }
        c = (char)i;
        bytesProcessed++;
        switch (c)
        {
        case '\r': *buffer++ = 0; break;
        case '\n':
            *buffer++ = 0;
            done      = true;
            break;
        default: *buffer++ = c; break;
        }

        if (bytesProcessed == size)
        {
            break;
        }
    }

    *buffer = 0;
    return bytesProcessed;
}

//============================================================================
//
//============================================================================

void TCPConnection::Tick()
{
    int         count;
    int         i;
    DataBuffer* buffer;
    uint32_t    currentTime_us;
    uint32_t    timeoutTime_us;

    HoldingQueueLock.Take(__FILE__, __LINE__);
    count          = HoldingQueue.GetCount();
    currentTime_us = (int32_t)osTime::GetTime();

    // Check for retransmit timeout
    timeoutTime_us = currentTime_us - TCP_RETRANSMIT_TIMEOUT_US;
    for (i = 0; i < count; i++)
    {
        buffer = (DataBuffer*)HoldingQueue.Get();
        if ((int32_t)(buffer->Time_us - timeoutTime_us) <= 0)
        {
            printf("TCP retransmit timeout %u, %u, delta %d\n",
                   buffer->Time_us,
                   timeoutTime_us,
                   (int32_t)(buffer->Time_us - timeoutTime_us));
            buffer->Time_us = currentTime_us;
            IP->Retransmit(buffer);
        }

        HoldingQueue.Put(buffer);
    }
    HoldingQueueLock.Give();

    // Check for TIMED_WAIT timeouts
    for (i = 0; i < TCP_MAX_CONNECTIONS; i++)
    {
        if (TCP->ConnectionList[i].State == TIMED_WAIT)
        {
            if (currentTime_us - TCP->ConnectionList[i].Time_us >= TCP_TIMED_WAIT_TIMEOUT_US)
            {
                TCP->ConnectionList[i].State = CLOSED;
            }
        }
    }

    // Check for delayed ACK
    if (LastAck != AcknowledgementNumber)
    {
        SendFlags(FLAG_ACK);
    }
}

//============================================================================
//
//============================================================================

void TCPConnection::CalculateRTT(int32_t M)
{
    int32_t err;

    err = M - RTT_us;

    // Gain is 0.125
    RTT_us = RTT_us + (125 * err) / 1000;

    if (err < 0)
    {
        err = -err;
    }

    // Gain is 0.250
    RTTDeviation = RTTDeviation + (250 * (err - RTTDeviation)) / 1000;
}

//============================================================================
//
//============================================================================

void TCPConnection::StoreRxData(DataBuffer* buffer)
{
    uint16_t i;

    if (buffer->Length > CurrentWindow)
    {
        printf("Rx window overrun, buffer %d, window %d\n", buffer->Length, CurrentWindow);
        return;
    }

    for (i = 0; i < buffer->Length; i++)
    {
        RxBuffer[RxInOffset++] = buffer->Packet[i];
        if (RxInOffset >= TCP_RX_WINDOW_SIZE)
        {
            RxInOffset = 0;
        }
    }
    CurrentWindow -= buffer->Length;
    RxBufferEmpty = false;
}

//============================================================================
//
//============================================================================

const char* TCPConnection::GetStateString() const
{
    const char* rc;
    switch (State)
    {
    case TCPConnection::CLOSED: rc       = "CLOSED"; break;
    case TCPConnection::LISTEN: rc       = "LISTEN"; break;
    case TCPConnection::SYN_SENT: rc     = "SYN_SENT"; break;
    case TCPConnection::SYN_RECEIVED: rc = "SYN_RECEIVED"; break;
    case TCPConnection::ESTABLISHED: rc  = "ESTABLISHED"; break;
    case TCPConnection::FIN_WAIT_1: rc   = "FIN_WAIT_1"; break;
    case TCPConnection::FIN_WAIT_2: rc   = "FIN_WAIT_2"; break;
    case TCPConnection::CLOSE_WAIT: rc   = "CLOSE_WAIT"; break;
    case TCPConnection::CLOSING: rc      = "CLOSING"; break;
    case TCPConnection::LAST_ACK: rc     = "LAST_ACK"; break;
    case TCPConnection::TIMED_WAIT: rc   = "TIMED_WAIT"; break;
    case TCPConnection::TTCP_PERSIST: rc = "TTCP_PERSIST"; break;
    }
    return rc;
}
