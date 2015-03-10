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
