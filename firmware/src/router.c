/**
  Copyright (c) 2015, OLogN Technologies AG. All rights reserved.

  Redistribution and use of this file in source (.rst) and compiled
  (.html, .pdf, etc.) forms, with or without modification, are permitted
  provided that the following conditions are met:
      * Redistributions in source form must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in compiled form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the OLogN Technologies AG nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE
 */

#include "router.h"

static RouterPacket _inRP;
static uint8_t _newInRPReady = 0;
static uint8_t _inBuffer[BUFFER_IN_LEN] = {0};
static RouterPacketOutStack _outRPStack[BUFFER_OUT_LEN] = {{0}};

void routerInit()
{
    UARTInit(ROUTER_UART_SPEED);
}

void routerLoop()
{
    _newInRPReady = 0;

    if (BUFFER_OUT_LEN)
        _routerResendOutPackets();

    int16_t _rxByte;
    while (!_newInRPReady && (_rxByte = UARTReceiveByte()) != -1)
        _routerOnByteReceived(_rxByte);

    if (_newInRPReady && _inRP.destination == DEVICE_ID \
         && _inRP.satpFlags & SATP_FLAG_ACK)
        _routerAcknowledgeInPacket();
}

uint8_t routerHasInPacket()
{
    return _newInRPReady;
}

RouterPacket *routerGetInPacket()
{
    return &_inRP;
}

void routerSendPacket(RouterPacket* outRP)
{
    UARTTransmitByte(PACKET_SOP_CODE);
    UARTTransmitByte(outRP->cdc);
    UARTTransmitByte(outRP->source);
    UARTTransmitByte(outRP->destination);

    uint8_t _flags = (outRP->satpFlags << 5) | outRP->dataLength;
    UARTTransmitByte(_flags);

    uint8_t i;
    for (i = 0; i < outRP->dataLength; i++)
        UARTTransmitByte(outRP->data[i]);

    /* calc CRC */
    outRP->crc = crc_update(0, &outRP->cdc, 1);
    outRP->crc = crc_update(outRP->crc, &outRP->source, 1);
    outRP->crc = crc_update(outRP->crc, &outRP->destination, 1);
    outRP->crc = crc_update(outRP->crc, &_flags, 1);
    outRP->crc = crc_update(outRP->crc, outRP->data, outRP->dataLength);
    UARTTransmitByte(outRP->crc >> 8);
    UARTTransmitByte(outRP->crc & 0xFF);

    UARTTransmitByte(PACKET_EOF_CODE);

    if (BUFFER_OUT_LEN && outRP->satpFlags & SATP_FLAG_ACK)
    {
        /* check if this packet already in buffer */
        for (i = 0; i < BUFFER_OUT_LEN; i++)
        {
            if (&_outRPStack[i].rp == outRP)
                return;
        }

        _routerShiftOutPacketStack();
        _outRPStack[0].rp = *outRP;
        _outRPStack[0].expireTime = getTimeMillis() + PACKET_OUT_TIMESTEP;
        _outRPStack[0].sentNums = 1;
    }
}

void routerAcknowledgeOutPacket(RouterPacket* outRP)
{
    uint8_t i;
    for (i = 0; i < BUFFER_OUT_LEN; i++)
    {
        if (!_outRPStack[i].sentNums ||
            _outRPStack[i].rp.destination != outRP->source ||
            _outRPStack[i].rp.source != outRP->destination ||
            _outRPStack[i].rp.crc != (outRP->data[0] << 8 | outRP->data[1]))
            continue;

        _outRPStack[i].sentNums = 0;
        _outRPStack[i].expireTime = 0;
    }
}

void _routerOnByteReceived(uint8_t inByte)
{
    _routerInBufferPushByte(inByte);

    if (inByte != PACKET_EOF_CODE)
        return;

    uint8_t i = BUFFER_IN_LEN - 1 - PACKET_CRC_LEN - PACKET_HEADER_LEN;
    for (; i > 0; i--)
    {
        if (_inBuffer[i-1] != PACKET_SOP_CODE)
            continue;

        if (_routerInBufferContainsPacket(i-1))
        {
            _routerParseInBufferPacket(i-1);
            _newInRPReady = 1;
            return;
        }
    }
}

void _routerInBufferPushByte(byte inByte)
{
    uint8_t i;
    for (i = 1; i < BUFFER_IN_LEN; i++)
        _inBuffer[i-1] = _inBuffer[i];

    _inBuffer[BUFFER_IN_LEN - 1] = inByte;
}

uint8_t _routerInBufferContainsPacket(uint8_t sopIndex)
{
    _inRP.dataLength = _inBuffer[sopIndex + PACKET_HEADER_LEN] & 0xF;

    if (_inRP.dataLength > PACKET_MAXDATA_LEN ||
        sopIndex + PACKET_HEADER_LEN + _inRP.dataLength \
        + PACKET_CRC_LEN + 2 != BUFFER_IN_LEN )
        return 0;

    _inRP.crc = _inBuffer[BUFFER_IN_LEN - PACKET_CRC_LEN - 1] << 8;
    _inRP.crc |= _inBuffer[BUFFER_IN_LEN - PACKET_CRC_LEN];

    return _inRP.crc == crc_update(
            0,
            &_inBuffer[sopIndex + 1],
            PACKET_HEADER_LEN + _inRP.dataLength);
}

void _routerParseInBufferPacket(uint8_t sopIndex)
{
    /* dataLen + crc already parsed in _routerInBufferContainsPacket */
    uint8_t i;
    for (i = 0; i < _inRP.dataLength; i++)
        _inRP.data[i] = _inBuffer[sopIndex + PACKET_HEADER_LEN + i + 1];

    _inRP.cdc = _inBuffer[sopIndex + 1];
    _inRP.source = _inBuffer[sopIndex + 2];
    _inRP.destination = _inBuffer[sopIndex + 3];
    _inRP.satpFlags = _inBuffer[sopIndex + 4] >> 5;
}

void _routerAcknowledgeInPacket()
{
    RouterPacket outRP;
    outRP.cdc = 0x0A;
    outRP.source = _inRP.destination;
    outRP.destination = _inRP.source;
    outRP.satpFlags = SATP_FLAG_FIN;
    outRP.dataLength = 2;
    outRP.data[0] = _inRP.crc >> 8;
    outRP.data[1] = _inRP.crc & 0xFF;

    routerSendPacket(&outRP);
}

void _routerShiftOutPacketStack()
{
    uint8_t i;
    for (i = BUFFER_OUT_LEN-1; i > 0; i--)
        _outRPStack[i] = _outRPStack[i-1];
}

void _routerResendOutPackets()
{
    uint32_t now = getTimeMillis();
    uint8_t i;
    for (i = 0; i < BUFFER_OUT_LEN; i++)
    {
        if (!_outRPStack[i].sentNums || _outRPStack[i].expireTime >= now)
            continue;

        _outRPStack[i].expireTime = now + \
                            (PACKET_OUT_TIMESTEP * _outRPStack[i].sentNums);
        _outRPStack[i].sentNums++;

        routerSendPacket(&_outRPStack[i].rp);
    }
}
