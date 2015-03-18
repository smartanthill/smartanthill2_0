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

#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "platform_tools.h"
#include "configuration.h"
#include "crc.h"

/* Based on /docs/specification/network/protocols/sarp.html */
#define PACKET_SOP_CODE         0x1
#define PACKET_HEADER_LEN       4
#define PACKET_MAXDATA_LEN      8
#define PACKET_CRC_LEN          2
#define PACKET_EOF_CODE         0x17
#define PACKET_OUT_TIMESTEP     1000 /* In milliseconds */

#define SATP_FLAG_SEG           0x4
#define SATP_FLAG_FIN           0x2
#define SATP_FLAG_ACK           0x1

#define BUFFER_IN_LEN           16 /* The sum of PACKET_* defines length */
#define BUFFER_OUT_LEN          5

typedef struct
{
    uint8_t cdc;
    uint8_t source;
    uint8_t destination;
    uint8_t satpFlags;
    uint8_t dataLength;
    uint8_t data[8];
    crc_t crc;

} RouterPacket;

typedef struct
{
    RouterPacket rp;
    uint32_t expireTime;
    uint8_t sentNums;

} RouterPacketOutStack;

#ifdef __cplusplus
extern "C" {
#endif

void routerInit();
void routerLoop();
uint8_t routerHasInPacket();
RouterPacket *routerGetInPacket();
void routerSendPacket(RouterPacket* outRP);
void routerAcknowledgeOutPacket(RouterPacket* outRP);

#ifdef __cplusplus
}
#endif

void _routerOnByteReceived(uint8_t inByte);
void _routerInBufferPushByte(uint8_t inByte);
uint8_t _routerInBufferContainsPacket(uint8_t sopIndex);
void _routerParseInBufferPacket(uint8_t sopIndex);
void _routerAcknowledgeInPacket();
void _routerShiftOutPacketStack();
void _routerResendOutPackets();

#endif
