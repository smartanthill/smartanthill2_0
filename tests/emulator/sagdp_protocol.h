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
#define SAGDP_STATE_IDLE 1
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
#define DATA_SAGDP_LSM_SIZE ( 1 + SAGDP_LRECEIVED_PID_SIZE + SAGDP_LSENT_PID_SIZE + SAGDP_LTO_SIZE ) // timer value


// SAGDP timer constants
// TODO: revise when values are finalized in the documentation
#define SAGDP_LTO_START 3 // treated as seconds; in general, by necessity, can be interpreted in times of basic time units 
#define SAGDP_LTO_EXP_TOP 3 // new_lto = lto * SAGDP_LTO_EXP_TOP / SAGDP_LTO_EXP_BOTTOM
#define SAGDP_LTO_EXP_BOTTOM 2
#define SAGDP_LTO_MAX 189

void setIniLTO( uint8_t* lto )
{
	*lto = SAGDP_LTO_START;
}

void cappedExponentiateLTO( uint8_t* lto )
{
	if ( *lto >= SAGDP_LTO_MAX ) return;
	uint16_t _lto = *lto;
	_lto *= SAGDP_LTO_EXP_TOP;
	_lto /= SAGDP_LTO_EXP_BOTTOM;
	*lto = (uint8_t)_lto;
}



uint8_t handlerSAGDP_timer( uint8_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
	return 0;
}

uint8_t handlerSAGDP_receiveNewUP( uint8_t* pid, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data )
{
	// sizeInOut represents a size of UP packet
	// A new packet can come either in idle (beginning of a chain), or in wait-remote (continuation of a chain) state.
	// As a result SAGDP changes its state to wait-local, or (in case of errors) to not-initialized state

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = *buffIn & SAGDP_P_STATUS_MASK; // get "our" bits

	if ( state == SAGDP_STATE_IDLE )
	{
#ifdef USED_AS_MASTER
		if ( packet_status == SAGDP_P_STATUS_ERROR_MSG )
		{
			// TODO: process
		}
		else if ( packet_status != SAGDP_P_STATUS_FIRST ) // unexpected state; silently ignore
		{
			return SAGDP_RET_OK; // just ignore
		}
#else // USED_AS_MASTER not ndefined
		if ( packet_status != SAGDP_P_STATUS_FIRST ) // invalid states
		{
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}
#endif
		else
		{
			memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, buffIn + 1, SAGDP_LRECEIVED_PID_SIZE );
			*sizeInOut -= 1 + SAGDP_LRECEIVED_PID_SIZE;
			memcpy( buffOut, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut );

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_LOCAL;
			return SAGDP_RET_TO_HIGHER;
		}
	}

	else if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
#ifdef USED_AS_MASTER
		if ( packet_status == SAGDP_P_STATUS_ERROR_MSG )
		{
			// TODO: process
		}
		else if ( packet_status == SAGDP_P_STATUS_FIRST ) // unexpected state; silently ignore
		{
			return SAGDP_RET_OK; // just ignore
		}
		else
		{
			bool isreply = memcmp( buffIn + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			if ( !isreply ) // silently ignore
			{
				return SAGDP_RET_OK;
			}
			// for non-terminating, save packet ID
			if ( packet_status == SAGDP_P_STATUS_INTERMEDIATE )
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, buffIn + 1, SAGDP_LRECEIVED_PID_SIZE );
			// form a packet for higher level
			*sizeInOut -= 1 + SAGDP_LRECEIVED_PID_SIZE;
			memcpy( buffOut, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut );

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_LOCAL;
			return SAGDP_RET_TO_HIGHER;
		}
#else // USED_AS_MASTER not ndefined
		if ( packet_status != SAGDP_P_STATUS_FIRST ) // invalid states
		{
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else
		{
			memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, buffIn + 1, SAGDP_LRECEIVED_PID_SIZE );
			memcpy( buffOut, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut );
			*sizeInOut -= 1 + SAGDP_LRECEIVED_PID_SIZE;

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_LOCAL;
			return SAGDP_RET_TO_HIGHER;
		}
#endif
	}

	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveRepeatedUP( uint8_t* pid, uint8_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// SAGDP can legitimately receive a repeated packet only if it is in wait-local state 
	// (as repeated can be received only after a "new", and receiving a "new" causes transition to either idle or wait-local state)
	// TODO: chack that it CANNOT be received in wait-remote state and correct documantation, if necessary
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		assert( *( data + DATA_SAGDP_STATE_OFFSET ) == SAGDP_STATE_WAIT_LOCAL ); // SAGDP does not change state its here
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveHLP( uint8_t packet_status, uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// Important: sizeInOut is a size of the message (not including any representation of its status in the chain); returned size: sizeInOut += 1 + SAGDP_LRECEIVED_PID_SIZE
	//
	// there are two states when SAGDP can legitimately receive a packet from a higher level: idle (packet is first in the chain), and wait-local (any subsequent packet)
	// the packet is processed and passed for further sending; SAGDP waits for its PID and thus transits to SAGDP_STATE_WAIT_PID
	//
	// It is a responsibility of a higher level to report the status of a packet. 
	//

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_IDLE )
	{
		if ( ( packet_status & SAGDP_P_STATUS_FIRST ) == 0 || ( packet_status & SAGDP_P_STATUS_TERMINATING ) )
		{
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}

		// form a UP packet
		uint8_t* writeptr = buffOut;
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		*(writeptr++) = ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn, *sizeInOut );
		*sizeInOut += 1 + SAGDP_LRECEIVED_PID_SIZE;

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		if ( packet_status & SAGDP_P_STATUS_FIRST )
		{
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}

		// form a UP packet
		uint8_t* writeptr = buffOut;
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		*(writeptr++) = ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn, *sizeInOut );
		*sizeInOut += 1 + SAGDP_LRECEIVED_PID_SIZE;

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receivePID( uint8_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_PID )
	{
		memcpy( data + DATA_SAGDP_LSENT_PID_OFFSET, buffIn, SAGDP_LSENT_PID_SIZE );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
		return SAGDP_RET_OK;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
	// TODO: ensure no other special cases
}


#endif // __SAGDP_PROTOCOL_H__