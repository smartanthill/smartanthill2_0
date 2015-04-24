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


#if !defined __SAGDP_PROTOCOL_H__
#define __SAGDP_PROTOCOL_H__

#include "sa-common.h"
//#include "sa-eeprom.h"
#include "zepto-mem-mngmt.h"


// RET codes
#define SAGDP_RET_SYS_CORRUPTED 0 // data processing inconsistency detected
#define SAGDP_RET_OK 1 // no output is available and no further action is required (for instance, after getting PID)
#define SAGDP_RET_TO_LOWER_NEW 2 // new packet
#define SAGDP_RET_TO_LOWER_REPEATED 3 // repeated packet
#define SAGDP_RET_TO_LOWER_NONE 4 // repeated packet
#define SAGDP_RET_TO_HIGHER 5
#define SAGDP_RET_TO_HIGHER_ERROR 6
#define SAGDP_RET_START_OVER_FIRST_RECEIVED 7 // "first" packet is received while SAGDP is in "wait-remote" state
#define SAGDP_RET_NEED_NONCE 8 // nonce is necessary before sending a packet


// SAGDP States
#define SAGDP_STATE_NOT_INITIALIZED 0
#define SAGDP_STATE_IDLE 0 // TODO: implement transition from non_initialized to idle state or ensure there is no difference in states
#define SAGDP_STATE_WAIT_REMOTE 2
#define SAGDP_STATE_WAIT_LOCAL 3


// packet statuses within the chain
#define SAGDP_P_STATUS_INTERMEDIATE 0
#define SAGDP_P_STATUS_FIRST 1
#define SAGDP_P_STATUS_TERMINATING 2
#define SAGDP_P_STATUS_MASK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING )
#define SAGDP_P_STATUS_NO_RESEND 4
#define SAGDP_P_STATUS_ACK 8
#define SAGDP_P_STATUS_ERROR_MSG ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING )
#define SAGDP_P_STATUS_IS_ACK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING | SAGDP_P_STATUS_ACK )
#define SAGDP_P_STATUS_FULL_MASK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING | SAGDP_P_STATUS_NO_RESEND | SAGDP_P_STATUS_ACK )


// sizes
#define SAGDP_PACKETID_SIZE 6
#define SAGDP_LRECEIVED_PID_SIZE SAGDP_PACKETID_SIZE // last received packet unique identifier
#define SAGDP_LSENT_PID_SIZE SAGDP_PACKETID_SIZE // last sent packet ID
#define SAGDP_LTO_SIZE 1 // length of the last timeout


// data structure / offsets
#define DATA_SAGDP_SIZE ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE + 2 )
#define DATA_SAGDP_STATE_OFFSET 0 // SAGDP state
#define DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET 1 // last received packet unique identifier
#define DATA_SAGDP_LRECEIVED_PID_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE ) // last received packet unique identifier
#define DATA_SAGDP_FIRST_LSENT_PID_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE ) // last sent packet ID
#define DATA_SAGDP_NEXT_LSENT_PID_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE ) // last sent packet ID
#define DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE ) // last sent packet ID
#define DATA_SAGDP_LTO_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE ) // timer value
//#define DATA_SAGDP_LSM_SIZE_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE ) // last sent packet size, 2 bytes
//#define DATA_SAGDP_WAS_PREV ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE + 1 ) // last sent packet size, 2 bytes
#define DATA_SAGDP_WAS_PREV ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE ) // last sent packet size, 2 bytes

// handlers
void sagdp_init( uint8_t* data );
/*
uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* nonce, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveUP( uint8_t* timeout, uint8_t* nonce, uint8_t* pid, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
*/
uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* nonce, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSAGDP_receiveUP( uint8_t* timeout, uint8_t* nonce, uint8_t* pid, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data );

#endif // __SAGDP_PROTOCOL_H__