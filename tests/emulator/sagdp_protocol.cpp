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

#include "sagdp_protocol.h"

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

void cancelLTO( uint8_t* lto )
{
	*lto = 0;
}



uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // other states: ignore
	{
		return SAGDP_RET_OK;
	}
}

uint8_t handlerSAGDP_receiveNewUP( uint8_t* timeout, uint8_t* pid, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data )
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
		else // allowed combination: packet_status == SAGDP_P_STATUS_FIRST in SAGDP_STATE_IDLE
		{
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = packet_status;
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );

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
		else // allowed combination: packet_status == SAGDP_P_STATUS_INTERMEDIATE in SAGDP_STATE_WAIT_REMOTE
		{
			uint8_t* pidlsent = data + DATA_SAGDP_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent         : %x%x%x%x%x%x\n", pidlsent[0], pidlsent[1], pidlsent[2], pidlsent[3], pidlsent[4], pidlsent[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			bool isreply = memcmp( buffIn + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			if ( !isreply ) // silently ignore
			{
				return SAGDP_RET_OK;
			}
			// for non-terminating, save packet ID
			if ( packet_status == SAGDP_P_STATUS_INTERMEDIATE )
			{
				PRINTF( "handlerSAGDP_receiveHLP(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
			}
			// form a packet for higher level
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = packet_status;
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );

			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#else // USED_AS_MASTER not ndefined
		if ( packet_status == SAGDP_P_STATUS_ERROR_MSG || packet_status == SAGDP_P_STATUS_FIRST )
		{
			// TODO: send an error message to a communication partner
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else // intermediate or terminating
		{
			uint8_t* pidlsent = data + DATA_SAGDP_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent         : %x%x%x%x%x%x\n", pidlsent[0], pidlsent[1], pidlsent[2], pidlsent[3], pidlsent[4], pidlsent[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			bool isreply = memcmp( buffIn + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			if ( !isreply ) // silently ignore
			{
				// TODO: send an error message to a communication partner
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				return SAGDP_RET_SYS_CORRUPTED;
			}
			// for non-terminating, save packet ID
			if ( packet_status == SAGDP_P_STATUS_INTERMEDIATE )
			{
				PRINTF( "handlerSAGDP_receiveHLP(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
			}
			// form a packet for higher level
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = packet_status;
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );

			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
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

uint8_t handlerSAGDP_receiveRepeatedUP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// SAGDP can legitimately receive a repeated packet in wait-remote state (the other side sounds like "we have not received anything from you; please resend, only then we will probably send you something new")
	// LSP must be resent
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// SAGDP can legitimately receive a repeated packet in wait-remote state (the other side sounds like "we have not received anything from you; please resend, only then we will probably send you something new")
	// LSP must be resent
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint16_t* sizeInOut, uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// Important: sizeInOut is a size of the message; returned size: sizeInOut += SAGDP_LRECEIVED_PID_SIZE
	//
	// there are two states when SAGDP can legitimately receive a packet from a higher level: idle (packet is first in the chain), and wait-local (any subsequent packet)
	// the packet is processed and passed for further sending; SAGDP waits for its PID and thus transits to SAGDP_STATE_WAIT_PID
	//
	// It is a responsibility of a higher level to report the status of a packet. 
	//

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = buffIn[0];

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
		uint8_t* pid = data + DATA_SAGDP_LRECEIVED_PID_OFFSET;
		PRINTF( "handlerSAGDP_receiveHLP(): PID reply-to (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn+1, *sizeInOut );
		*sizeInOut += SAGDP_LRECEIVED_PID_SIZE;

		// save a copy
		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = *sizeInOut;
		memcpy( lsm, buffOut, *sizeInOut );

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

//		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
		*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_IDLE : SAGDP_STATE_WAIT_PID;
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
		uint8_t* pid = data + DATA_SAGDP_LRECEIVED_PID_OFFSET;
		PRINTF( "handlerSAGDP_receiveHLP(): PID reply-to: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn+1, *sizeInOut );
		*sizeInOut += SAGDP_LRECEIVED_PID_SIZE;

		// save a copy
		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = *sizeInOut;
		memcpy( lsm, buffOut, *sizeInOut );

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

//		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
		*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_IDLE : SAGDP_STATE_WAIT_PID;
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receivePID( uint8_t* pid, uint8_t* data )
{
	PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_PID )
	{
		memcpy( data + DATA_SAGDP_LSENT_PID_OFFSET, pid, SAGDP_LSENT_PID_SIZE );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
		return SAGDP_RET_OK;
	}
	else if ( state == SAGDP_STATE_IDLE ) // ignore
	{
		return SAGDP_RET_OK;
	}
	else // invalid states
	{
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
	// TODO: ensure no other special cases
}
