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

#include "osm_states.h"

void osmStateAcknowledgeOutPacket()
{
    routerAcknowledgeOutPacket(routerGetInPacket());
    osm.makeTransition(IDLE_STATE);
}

void osmStateListOperations()
{

    RouterPacket *inPacket = routerGetInPacket();
    RouterPacket outPacket;
    outPacket.cdc = 0xC9;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = ((inPacket->satpFlags & SATP_FLAG_ACK)?
                            SATP_FLAG_ACK : 0);
    outPacket.dataLength = 0;

    uint8_t nums = osmGetStateNums() - 1 /* skip SEGMENT_ACKNOWLEDGMENT */;

    if (nums > 8)
    {
        outPacket.satpFlags |= SATP_FLAG_SEG;
        outPacket.data[0] = 0;
        outPacket.dataLength = 1;
    }

    while (nums--)
    {
        if (outPacket.dataLength == 8)
        {
            routerSendPacket(&outPacket);
            outPacket.data[0]++;
            outPacket.dataLength = 1;
        }

        outPacket.data[outPacket.dataLength] = osmGetStateCDCByIndex(nums);
        outPacket.dataLength++;
    }

    outPacket.satpFlags |= SATP_FLAG_FIN;
    routerSendPacket(&outPacket);

    osm.makeTransition(IDLE_STATE);
}

void osmStateConfigurePinMode()
{
    RouterPacket *inPacket = routerGetInPacket();
    RouterPacket outPacket;
    outPacket.cdc = 0xCA;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = inPacket->satpFlags;
    outPacket.dataLength = 0;

    /* The last unused byte from previous Packet */
    static uint8_t unusedDataByte = 0;
    uint8_t i = 0;
    uint8_t pin = 0;

    if (inPacket->satpFlags & SATP_FLAG_SEG)
    {
        /* reuse segment order */
        outPacket.data[0] = inPacket->data[0];
        outPacket.dataLength = 1;

        if (inPacket->dataLength == PACKET_MAXDATA_LEN)
        {
            i = 1;
            unusedDataByte = 0;
        }
    }

    if (inPacket->dataLength >= i+2)
    {
        for (; i < inPacket->dataLength-1; i+=2)
        {
            if (i == 0 && unusedDataByte)
                pin = unusedDataByte;
            else
                pin = inPacket->data[i];

            configurePinMode(pin, inPacket->data[i+1]);

            /* collect configured pins */
            outPacket.data[outPacket.dataLength] = pin;
            outPacket.dataLength++;
        }
    }

    if (inPacket->satpFlags & SATP_FLAG_SEG &&
            inPacket->dataLength == PACKET_MAXDATA_LEN)
        unusedDataByte = inPacket->data[PACKET_MAXDATA_LEN-1];

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}

void osmStateReadDigitalPin()
{
    RouterPacket *inPacket = routerGetInPacket();

    RouterPacket outPacket;
    outPacket.cdc = 0xCB;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = inPacket->satpFlags;
    outPacket.dataLength = inPacket->dataLength;

    uint8_t i = 0;

    if (inPacket->satpFlags & SATP_FLAG_SEG)
    {
        /* reuse segment order */
        outPacket.data[0] = inPacket->data[0];
        i = 1;
    }

    for (; i < inPacket->dataLength; i++)
        outPacket.data[i] = readDigitalPin(inPacket->data[i]);

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}

void osmStateWriteDigitalPin()
{
    RouterPacket *inPacket = routerGetInPacket();
    RouterPacket outPacket;
    outPacket.cdc = 0xCC;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = inPacket->satpFlags;
    outPacket.dataLength = 0;

    /* The last unused byte from previous Packet */
    static uint8_t unusedDataByte = 0;
    uint8_t i = 0;
    uint8_t pin = 0;

    if (inPacket->satpFlags & SATP_FLAG_SEG)
    {
        /* reuse segment order */
        outPacket.data[0] = inPacket->data[0];
        outPacket.dataLength++;

        if (inPacket->dataLength == PACKET_MAXDATA_LEN)
        {
            i = 1;
            unusedDataByte = 0;
        }
    }

    if (inPacket->dataLength >= i+2)
    {
        for (; i < inPacket->dataLength-1; i+=2)
        {
            if (i == 0 && unusedDataByte)
                pin = unusedDataByte;
            else
                pin = inPacket->data[i];

            writeDigitalPin(pin, inPacket->data[i+1]);

            /* collect updated pins */
            outPacket.data[outPacket.dataLength] = pin;
            outPacket.dataLength++;
        }
    }

    if (inPacket->satpFlags & SATP_FLAG_SEG &&
            inPacket->dataLength == PACKET_MAXDATA_LEN)
        unusedDataByte = inPacket->data[PACKET_MAXDATA_LEN-1];

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}

void osmStateConfigureAnalogReference()
{
    RouterPacket *inPacket = routerGetInPacket();

    if (inPacket->dataLength != 1)
        return;

    configureAnalogReference(inPacket->data[0]);

    RouterPacket outPacket;
    outPacket.cdc = 0xCD;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = ((inPacket->satpFlags & SATP_FLAG_ACK)?
                            SATP_FLAG_FIN | SATP_FLAG_ACK : SATP_FLAG_FIN);
    outPacket.dataLength = 1;
    outPacket.data[0] = 0x01;

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}

void osmStateReadAnalogPin()
{
    RouterPacket *inPacket = routerGetInPacket();

    RouterPacket outPacket;
    outPacket.cdc = 0xCE;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = ((inPacket->satpFlags & SATP_FLAG_ACK)?
                            SATP_FLAG_ACK : 0);
    outPacket.dataLength = 0;

    static uint8_t sessionSegmentOrder = 0;
    uint8_t i = (inPacket->satpFlags & SATP_FLAG_SEG)? 1 : 0;
    uint16_t _value = 0;

    if (inPacket->dataLength > 4 || sessionSegmentOrder)
    {
        outPacket.satpFlags |= SATP_FLAG_SEG;
        outPacket.data[0] = sessionSegmentOrder;
        outPacket.dataLength = 1;
    }

    for (; i < inPacket->dataLength; i++)
    {
        if (outPacket.dataLength > 6)
        {
            routerSendPacket(&outPacket);

            sessionSegmentOrder++;
            outPacket.data[0] = sessionSegmentOrder;
            outPacket.dataLength = 1;
        }

        _value = readAnalogPin(inPacket->data[i]);

        outPacket.data[outPacket.dataLength] = _value >> 8;
        outPacket.data[outPacket.dataLength+1] = _value & 0xFF;
        outPacket.dataLength += 2;
    }

    if (inPacket->satpFlags & SATP_FLAG_FIN)
    {
        outPacket.satpFlags |= SATP_FLAG_FIN;
        sessionSegmentOrder = 0;
    }

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}

void osmStateWriteAnalogPin()
{
    RouterPacket *inPacket = routerGetInPacket();
    RouterPacket outPacket;
    outPacket.cdc = 0xCF;
    outPacket.source = inPacket->destination;
    outPacket.destination = inPacket->source;
    outPacket.satpFlags = inPacket->satpFlags;
    outPacket.dataLength = 0;

    /* The last unused byte from previous Packet */
    static uint8_t unusedDataByte = 0;
    uint8_t i = 0;
    uint8_t pin = 0;

    if (inPacket->satpFlags & SATP_FLAG_SEG)
    {
        /* reuse segment order */
        outPacket.data[0] = inPacket->data[0];
        outPacket.dataLength++;

        if (inPacket->dataLength == PACKET_MAXDATA_LEN)
        {
            i = 1;
            unusedDataByte = 0;
        }
    }

    if (inPacket->dataLength >= i+2)
    {
        for (; i < inPacket->dataLength-1; i+=2)
        {
            if (i == 0 && unusedDataByte)
                pin = unusedDataByte;
            else
                pin = inPacket->data[i];

            writeAnalogPin(pin, inPacket->data[i+1]);

            /* collect updated pins */
            outPacket.data[outPacket.dataLength] = pin;
            outPacket.dataLength++;
        }
    }

    if (inPacket->satpFlags & SATP_FLAG_SEG &&
            inPacket->dataLength == PACKET_MAXDATA_LEN)
        unusedDataByte = inPacket->data[PACKET_MAXDATA_LEN-1];

    routerSendPacket(&outPacket);
    osm.makeTransition(IDLE_STATE);
}
