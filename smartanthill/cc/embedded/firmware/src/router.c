/**
  Copyright (C) 2015 OLogN Technologies AG

  This source file is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
