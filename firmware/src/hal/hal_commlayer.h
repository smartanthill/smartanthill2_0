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

#include "../common/sa_common.h"
#include "zepto_mem_mngmt_hal_spec.h"
#include "../common/zepto_mem_mngmt.h"

// RET codes
#define COMMLAYER_RET_FAILED 0
#define COMMLAYER_RET_OK 1
#define COMMLAYER_RET_PENDING 2

#define COMMLAYER_RET_FROM_CENTRAL_UNIT 10
#define COMMLAYER_RET_FROM_DEV 11
#define COMMLAYER_RET_TIMEOUT 12

#define COMMLAYER_RET_FROM_COMMM_STACK 10

#ifdef __cplusplus
extern "C" {
#endif

bool communication_initialize();
void communication_terminate();
uint8_t send_message( MEMORY_HANDLE mem_h );
uint8_t try_get_message( MEMORY_HANDLE mem_h );

#ifdef __cplusplus
}
#endif

//uint8_t wait_for_communication_event( MEMORY_HANDLE mem_h, uint16_t timeout );
//uint8_t wait_for_communication_event( uint16_t timeout );
#ifdef USED_AS_MASTER
#ifdef USED_AS_MASTER_COMMSTACK
uint8_t send_to_central_unit( MEMORY_HANDLE mem_h );
#elif defined USED_AS_MASTER_CORE
uint8_t send_to_commm_stack( MEMORY_HANDLE mem_h );
#else
#error unknown configuration
#endif
#endif



#endif // __SA_COMMLAYER_H__