/*******************************************************************************
Copyright (C) 2015 OLogN Technologies AG

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/

#if !defined __SA_COMMLAYER_H__
#define __SA_COMMLAYER_H__

#include "zepto-mem-mngmt.h"


// RET codes
#define COMMLAYER_RET_FAILED 0 // not authenticated, etc
#define COMMLAYER_RET_OK 1 // new packet
#define COMMLAYER_RET_PENDING 2


bool communicationInitializeAsServer();
bool communicationInitializeAsClient();
void communicationTerminate();
uint8_t sendMessage( uint16_t* msgSize, const uint8_t * buff );
uint8_t getMessage( uint16_t* msgSize, uint8_t * buff, int maxSize ); // returns when a packet received
uint8_t tryGetMessage(uint16_t* msgSize, uint8_t * buff, int maxSize); // returns immediately, but a packet reception is not guaranteed

uint8_t sendMessage( MEMORY_HANDLE mem_h );
uint8_t getMessage( MEMORY_HANDLE mem_h ); // returns when a packet received
uint8_t tryGetMessage( MEMORY_HANDLE mem_h ); // returns immediately, but a packet reception is not guaranteed

#endif // __SA_COMMLAYER_H__