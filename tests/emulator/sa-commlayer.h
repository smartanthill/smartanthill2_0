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

bool communication_initialize();
void communication_terminate();


// RET codes
#define COMMLAYER_RET_FAILED 0
#define COMMLAYER_RET_OK 1
#define COMMLAYER_RET_PENDING 2

#define COMMLAYER_RET_FROM_CENTRAL_UNIT 10
#define COMMLAYER_RET_FROM_DEV 11
#define COMMLAYER_RET_TIMEOUT 12

#define COMMLAYER_RET_FROM_COMMM_STACK 10



uint8_t sendMessage( MEMORY_HANDLE mem_h );
uint8_t tryGetMessage( MEMORY_HANDLE mem_h ); // returns immediately, but a packet reception is not guaranteed

uint8_t wait_for_communication_event( MEMORY_HANDLE mem_h, uint16_t timeout );
#ifdef USED_AS_MASTER
#ifdef USED_AS_MASTER_COMMSTACK
uint8_t send_to_central_unit( MEMORY_HANDLE mem_h );
#else
uint8_t send_to_commm_stack( MEMORY_HANDLE mem_h );
#endif
#endif



#endif // __SA_COMMLAYER_H__