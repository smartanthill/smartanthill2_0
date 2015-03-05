/*******************************************************************************
    Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source and compiled
    forms, with or without modification, are permitted
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
*******************************************************************************/

#if !defined __SAGDP_PROTOCOL_H__
#define __SAGDP_PROTOCOL_H__

#include "sa-common.h"
//#include "sa-eeprom.h"


// RET codes
#define SAGDP_RET_SYS_CORRUPTED 0 // data processing inconsistency detected
#define SAGDP_RET_OK 1 // no output is available and no further action is required (for instance, after getting PID)
#define SAGDP_RET_TO_LOWER_NEW 2 // new packet
#define SAGDP_RET_TO_LOWER_REPEATED 3 // repeated packet
#define SAGDP_RET_TO_HIGHER 4 // for error messaging


// SAGDP States
#define SAGDP_STATE_NOT_INITIALIZED 0
#define SAGDP_STATE_IDLE 0 // TODO: implement transition from non_initialized to idle state or ensure there is no difference in states
#define SAGDP_STATE_WAIT_PID 2
#define SAGDP_STATE_WAIT_REMOTE 3
#define SAGDP_STATE_WAIT_LOCAL 4


// packet statuses
#define SAGDP_P_STATUS_INTERMEDIATE 0
#define SAGDP_P_STATUS_FIRST 1
#define SAGDP_P_STATUS_TERMINATING 2
#define SAGDP_P_STATUS_ERROR_MSG ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING )
#define SAGDP_P_STATUS_REQUESTED_RESEND 4
#define SAGDP_P_STATUS_MASK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING | SAGDP_P_STATUS_REQUESTED_RESEND )


// sizes
#define SAGDP_PACKETID_SIZE 6
#define SAGDP_LRECEIVED_PID_SIZE SAGDP_PACKETID_SIZE // last received packet unique identifier
#define SAGDP_LSENT_PID_SIZE SAGDP_PACKETID_SIZE // last sent packet ID
#define SAGDP_LTO_SIZE 1 // length of the last timeout


// data structure / offsets
#define DATA_SAGDP_SIZE (1+SAGDP_LRECEIVED_PID_SIZE+SAGDP_LSENT_PID_SIZE+SAGDP_LTO_SIZE+2)
#define DATA_SAGDP_STATE_OFFSET 0 // SAGDP state
#define DATA_SAGDP_LRECEIVED_PID_OFFSET 1 // last received packet unique identifier
#define DATA_SAGDP_LSENT_PID_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE ) // last sent packet ID
#define DATA_SAGDP_LTO_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE ) // timer value
#define DATA_SAGDP_LSM_SIZE_OFFSET ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE ) // last sent packet size


// handlers
uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveNewUP( uint8_t* timeout, uint8_t* pid, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSAGDP_receiveRepeatedUP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm );
uint8_t handlerSAGDP_receivePID( uint8_t* PID, uint8_t* data );


#endif // __SAGDP_PROTOCOL_H__