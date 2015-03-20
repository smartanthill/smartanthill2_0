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

int8_t pid_compare( const uint8_t* pid1, const uint8_t* pid2 )
{
	int8_t i;
	for ( i=SAGDP_PACKETID_SIZE-1; i>=0; i-- )
	{
		if ( pid1[i] > pid2[i] ) return int8_t(1);
		if ( pid1[i] < pid2[i] ) return int8_t(-1);
	}
	return 0;
}

bool is_pid_in_range( const uint8_t* pid, const uint8_t* first_pid, const uint8_t* last_pid )
{
	return pid_compare( pid, first_pid ) >=0 && pid_compare( pid, last_pid ) <= 0;
}

bool is_pid_zero( const uint8_t* pid )
{
	return pid[0] == 0 && pid[1] == 0 && pid[2] == 0 && pid[3] == 0 && pid[4] == 0 && pid[5] == 0;
}
/*
bool is_diff_chain( const uint8_t* pid1, const uint8_t* pid2 )
{
	if ( ( pid1[0] & 0x80 ) != ( pid2[0] & 0x80 ) )
		return true;
	if ( ( pid1[0] & 0x80 ) != ( pid2[0] & 0x80 ) )
		return true;
	int8_t i;
	for ( i=SAGDP_PACKETID_SIZE-1; i>0; i-- )
		if ( pid1[i] != pid2[i] ) return true;
}
*/






void sagdp_init( uint8_t* data )
{
	memset( data, 0, DATA_SAGDP_SIZE );
//	memset( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, 0xff, SAGDP_PACKETID_SIZE );
}

uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* nonce, uint16_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		INCREMENT_COUNTER( 20, "handlerSAGDP_timer(), packet resent" );
		assert( nonce != NULL );
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // other states: ignore
	{
		return SAGDP_RET_OK;
	}
}

uint8_t handlerSAGDP_receiveUP( uint8_t* timeout, uint8_t* nonce, uint8_t* pid, uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// sizeInOut represents a size of UP packet
	// A new packet can come either in idle (beginning of a chain), or in wait-remote (continuation of a chain) state.
	// As a result SAGDP changes its state to wait-local, or (in case of errors) to not-initialized state

	INCREMENT_COUNTER( 21, "handlerSAGDP_receiveUP()" );

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = *buffIn & SAGDP_P_STATUS_FULL_MASK; // get "our" bits
	PRINTF( "handlerSAGDP_receiveUP(): state: %d, packet_status: %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 22, "handlerSAGDP_receiveUP(), idle, error message" );
			// TODO: process
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			if ( *sizeInOut > 1 + SAGDP_LRECEIVED_PID_SIZE )
			{
				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );
			}
			else
				*sizeInOut = 1;

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
				INCREMENT_COUNTER( 23, "handlerSAGDP_receiveUP(), idle, is-old" );
				PRINTF( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 24, "handlerSAGDP_receiveUP(), idle, is-old intermediate" );

					PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

					// re-send LSP
					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					// SAGDP status remains the same
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 25, "handlerSAGDP_receiveUP(), idle, is-old terminating" );
					assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					return SAGDP_RET_OK; // ignored
				}
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply ) // above the range; silently ignore
			{
				INCREMENT_COUNTER( 26, "handlerSAGDP_receiveUP(), idle, too old, ignored" );
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK;
			}
			INCREMENT_COUNTER( 27, "handlerSAGDP_receiveUP(), idle, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				return SAGDP_RET_OK; // ignored
			}
			else
			{
				assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
				return SAGDP_RET_OK; // ignored
			}
		}
#else // USED_AS_MASTER not ndefined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG ) // unexpected at slave's side
		{
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 22, "handlerSAGDP_receiveUP(), idle, error message" );
			// send an error message to a communication partner and reinitialize
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
//			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
//			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
//			return SAGDP_RET_SYS_CORRUPTED;
			uint8_t* pidprevlsent_first = data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = (!is_pid_zero(data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET)) && pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0  && pid_compare( buffIn + 1, data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET ) >= 0;
			if ( isold )
			{
				INCREMENT_COUNTER( 23, "handlerSAGDP_receiveUP(), idle, is-old" );
				PRINTF( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					// re-send LSP
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 24, "handlerSAGDP_receiveUP(), idle, is-old intermediate" );

					PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 25, "handlerSAGDP_receiveUP(), idle, is-old terminating" );
					assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					// send an error message to a communication partner and reinitialize
					buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
					*sizeInOut = 1;
					// TODO: add other relevant data, if any, and update sizeInOut
					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
					PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_SYS_CORRUPTED;
				}
//				return handlerSAGDP_receiveRepeatedUP( timeout, sizeInOut, buffIn, buffOut, buffOutSize, stack, stackSize, data, lsm );
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 26, "handlerSAGDP_receiveUP(), idle, too old, sys corrupted" );
				// send an error message to a communication partner and reinitialize
				buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
				*sizeInOut = 1;
				// TODO: add other relevant data, if any, and update sizeInOut
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 27, "handlerSAGDP_receiveUP(), idle, intermediate, sys corrupted" );
				// send an error message to a communication partner and reinitialize
				buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
				*sizeInOut = 1;
				// TODO: add other relevant data, if any, and update sizeInOut
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			else
			{
				INCREMENT_COUNTER( 28, "handlerSAGDP_receiveUP(), idle, terminating, ignored" );
				assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
				return SAGDP_RET_OK; // ignored
			}
		}
#endif
		else // allowed combination: packet_status == SAGDP_P_STATUS_FIRST in SAGDP_STATE_IDLE
		{
			INCREMENT_COUNTER( 29, "handlerSAGDP_receiveUP(), idle, first" );
			assert( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST && state == SAGDP_STATE_IDLE );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			const uint8_t* this_chain_id = buffIn + 1;
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): this_chain_id: %x%x%x%x%x%x\n", this_chain_id[0], this_chain_id[1], this_chain_id[2], this_chain_id[3], this_chain_id[4], this_chain_id[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): prev_chain_id: %x%x%x%x%x%x\n", prev_chain_id[0], prev_chain_id[1], prev_chain_id[2], prev_chain_id[3], prev_chain_id[4], prev_chain_id[5] );
			bool is_resent = pid_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 30, "handlerSAGDP_receiveUP(), idle, first, resent" );

				PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

				// re-send LSP
				*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
				memcpy( buffOut, lsm, *sizeInOut );
				// SAGDP status remains the same
				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 31, "handlerSAGDP_receiveUP(), idle, first, new" );
				cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
				*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
				memcpy( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, this_chain_id, SAGDP_LRECEIVED_PID_SIZE );
				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );

				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_LOCAL;
				return SAGDP_RET_TO_HIGHER;
			}
		}
	}

	else if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 40, "handlerSAGDP_receiveUP(), wait-remote, error" );
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			if ( *sizeInOut > 1 + SAGDP_LRECEIVED_PID_SIZE )
			{
				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );
			}
			else
				*sizeInOut = 1;

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			const uint8_t* this_chain_id = buffIn + 1;
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			bool is_resent = pid_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;

				INCREMENT_COUNTER( 42, "handlerSAGDP_receiveUP(), wait-remote, first, resent" );
				PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

				// re-send LSP
				*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
				memcpy( buffOut, lsm, *sizeInOut );
				// SAGDP status remains the same
				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 43, "handlerSAGDP_receiveUP(), wait-remote, first, new (ignored)" );
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_OK; // just ignore
				// TODO: form a packet
//				return SAGDP_RET_TO_HIGHER_ERROR;
			}
		}
		else
		{
			assert( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE || ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_TERMINATING );
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
//			bool isreply = memcmp( buffIn + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			bool isold = pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
//				return handlerSAGDP_receiveRepeatedUP( timeout, sizeInOut, buffIn, buffOut, buffOutSize, stack, stackSize, data, lsm );
				if ( ( buffIn[0] & SAGDP_P_STATUS_NO_RESEND ) == 0 )
				{
					PRINTF( "SAGDP: state = %d, packet_status = %d; isold\n", state, packet_status );
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 44, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend requested" );

					PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

//					*(data+DATA_SAGDP_ALREADY_REPLIED_OFFSET) = 1;
					cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
					*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					buffOut[0] |= SAGDP_P_STATUS_NO_RESEND;
					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend NOT requested" );
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, sizeInOut[0] & 3 );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply )
			{
				INCREMENT_COUNTER( 46, "handlerSAGDP_receiveUP(), wait-remote, !is-reply, ignored" );
				PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK; // silently ignore
				// TODO: form a packet
//				return SAGDP_RET_TO_HIGHER_ERROR;
			}
			INCREMENT_COUNTER( 47, "handlerSAGDP_receiveUP(), wait-remote, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				INCREMENT_COUNTER( 48, "handlerSAGDP_receiveUP(), wait-remote, other, intermediate" );
				PRINTF( "handlerSAGDP_receiveHLP(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
			}
			// form a packet for higher level
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );

			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*( data + DATA_SAGDP_STATE_OFFSET ) = ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#else // USED_AS_MASTER not defined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 40, "handlerSAGDP_receiveUP(), wait-remote, error" );
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// main question: is it a re-sent or a start of an actually new chain 
			bool current = pid_compare( buffIn + 1, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET ) == 0;
			if ( current )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 42, "handlerSAGDP_receiveUP(), wait-remote, first, resent" );

				PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

				*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
				memcpy( buffOut, lsm, *sizeInOut );
				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 43, "handlerSAGDP_receiveUP(), wait-remote, first, new (applied)" );
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_START_OVER_FIRST_RECEIVED;
			}
		}
		else // intermediate or terminating
		{
			assert( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST );
/*			uint8_t* pidlsent = data + DATA_SAGDP_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent         : %x%x%x%x%x%x\n", pidlsent[0], pidlsent[1], pidlsent[2], pidlsent[3], pidlsent[4], pidlsent[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			bool isreply = memcmp( buffIn + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;*/
			uint8_t* pidprevlsent_first = data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = (!is_pid_zero(data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET)) && pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0  && pid_compare( buffIn + 1, data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET ) >= 0;
			if ( isold )
			{
				if ( ( buffIn[0] & SAGDP_P_STATUS_NO_RESEND ) == 0 )
				{
					PRINTF( "SAGDP: state = %d, packet_status = %d; isold\n", state, packet_status );
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 44, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend requested" );

					PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

//					*(data+DATA_SAGDP_ALREADY_REPLIED_OFFSET) = 1;
					cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
					*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					buffOut[0] |= SAGDP_P_STATUS_NO_RESEND;
					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend NOT requested" );
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, sizeInOut[0] & 3 );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply ) // silently ignore
			{
				// send an error message to a communication partner and reinitialize
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 46, "handlerSAGDP_receiveUP(), wait-remote, !is-reply, ignored" );
				buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
				*sizeInOut = 1;
				// TODO: add other relevant data, if any, and update sizeInOut
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			INCREMENT_COUNTER( 47, "handlerSAGDP_receiveUP(), wait-remote, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				INCREMENT_COUNTER( 48, "handlerSAGDP_receiveUP(), wait-remote, other, intermediate" );
				PRINTF( "handlerSAGDP_receiveHLP(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
			}
			// form a packet for higher level
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );

			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*( data + DATA_SAGDP_STATE_OFFSET ) = ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#endif
	}

	else // invalid states
	{
		INCREMENT_COUNTER( 50, "handlerSAGDP_receiveUP(), invalid state" );
#if !defined USED_AS_MASTER
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		// send an error message to a communication partner and reinitialize
		buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
		*sizeInOut = 1;
#endif
		// TODO: add other relevant data, if any, and update sizeInOut
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint8_t* nonce, uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// SAGDP can legitimately receive a repeated packet in wait-remote state (the other side sounds like "we have not received anything from you; please resend, only then we will probably send you something new")
	// LSP must be resent

	*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
	if ( *sizeInOut == 0 )
	{
		INCREMENT_COUNTER( 63, "handlerSAGDP_receiveRequestResendLSP(), no lsm" );
		return SAGDP_RET_TO_LOWER_NONE;
	}

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		INCREMENT_COUNTER( 60, "handlerSAGDP_receiveRequestResendLSP(), wait-remote" );

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	if ( state == SAGDP_STATE_IDLE )
	{
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		INCREMENT_COUNTER( 61, "handlerSAGDP_receiveRequestResendLSP(), idle" );

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		INCREMENT_COUNTER( 62, "handlerSAGDP_receiveRequestResendLSP(), invalid state" );
		// send an error message to a communication partner and reinitialize
		buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
		*sizeInOut = 1;
		// TODO: add other relevant data, if any, and update sizeInOut
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint8_t* nonce, uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	// Important: sizeInOut is a size of the message; returned size: sizeInOut += SAGDP_LRECEIVED_PID_SIZE
	//
	// there are two states when SAGDP can legitimately receive a packet from a higher level: idle (packet is first in the chain), and wait-local (any subsequent packet)
	// the packet is processed and passed for further sending; SAGDP waits for its PID and thus transits to SAGDP_STATE_WAIT_PID
	//
	// It is a responsibility of a higher level to report the status of a packet. 
	//

	INCREMENT_COUNTER( 70, "handlerSAGDP_receiveHLP(), invalid state" );

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = buffIn[0];
	PRINTF( "handlerSAGDP_receiveHLP(): state = %d, packet_status = %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
		if ( ( packet_status & SAGDP_P_STATUS_FIRST ) == 0 || ( packet_status & SAGDP_P_STATUS_TERMINATING ) )
		{
#ifdef USED_AS_MASTER
			// TODO: should we do anything else but error reporting?
#else
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;

			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
#endif // USED_AS_MASTER
			INCREMENT_COUNTER( 71, "handlerSAGDP_receiveHLP(), idle, state/packet mismatch" );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		assert( ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_FIRST ); // in idle state we can expect only "first" packet
		assert( packet_status == SAGDP_P_STATUS_FIRST );

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		// "chain id" is shared between devices and therefore, should be unique for both sides, that is, shoud have master/slave distinguishing bit
		memcpy( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		*(data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET + SAGDP_PACKETID_SIZE - 1) |= ( MASTER_SLAVE_BIT << 7 );

		// form a UP packet
		uint8_t* writeptr = buffOut;
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		*(writeptr++) = ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn+1, *sizeInOut-1 );
		*sizeInOut += SAGDP_LRECEIVED_PID_SIZE;

		// save a copy
		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = *sizeInOut;
		memcpy( lsm, buffOut, *sizeInOut );

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

//		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
//		*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_WAIT_FIRST_PID_THEN_IDLE : SAGDP_STATE_WAIT_FIRST_PID_THEN_WR;
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
//		*(data+DATA_SAGDP_ALREADY_REPLIED_OFFSET) = 0;
		INCREMENT_COUNTER( 72, "handlerSAGDP_receiveHLP(), idle, PACKET=FIRST" );
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		if ( packet_status & SAGDP_P_STATUS_FIRST )
		{
#ifdef USED_AS_MASTER
			// TODO: should we do anything else but error reporting?
#else
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
#endif
			INCREMENT_COUNTER( 73, "handlerSAGDP_receiveHLP(), wait-remote, state/packet mismatch" );
			return SAGDP_RET_SYS_CORRUPTED;
		}

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		bool is_prev = pid_compare( data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET ) != 0;
		if ( is_prev )
			memcpy( data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE );
		else
			memcpy( data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		// form a UP packet
		uint8_t* writeptr = buffOut;
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		*(writeptr++) = ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );
		uint8_t* pid = data + DATA_SAGDP_LRECEIVED_PID_OFFSET;
		PRINTF( "handlerSAGDP_receiveHLP(): PID reply-to: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		memcpy( writeptr, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		writeptr += SAGDP_LRECEIVED_PID_SIZE;
		memcpy( writeptr, buffIn+1, *sizeInOut-1 );
		*sizeInOut += SAGDP_LRECEIVED_PID_SIZE;

		// save a copy
		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = *sizeInOut;
		memcpy( lsm, buffOut, *sizeInOut );

		// request set timer
		setIniLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);

//		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_PID;
//		*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_WAIT_FIRST_PID_THEN_IDLE : SAGDP_STATE_WAIT_FIRST_PID_THEN_WR;
		*( data + DATA_SAGDP_STATE_OFFSET ) = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_IDLE : SAGDP_STATE_WAIT_REMOTE;
//		*(data+DATA_SAGDP_ALREADY_REPLIED_OFFSET) = 0;
		INCREMENT_COUNTER( 74, "handlerSAGDP_receiveHLP(), wait-remote, intermediate/terminating" );
		INCREMENT_COUNTER_IF( 75, "handlerSAGDP_receiveHLP(), wait-remote, terminating", (packet_status >> 1) );
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else // invalid states
	{
#ifdef USED_AS_MASTER
		// TODO: should we do anything else but error reporting?
#else
			// send an error message to a communication partner and reinitialize
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		// send an error message to a communication partner and reinitialize
		buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
		*sizeInOut = 1;
		// TODO: add other relevant data, if any, and update sizeInOut
#endif
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		INCREMENT_COUNTER( 76, "handlerSAGDP_receiveHLP(), invalid state" );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}
