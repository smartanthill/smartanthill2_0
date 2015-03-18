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





void sagdp_init( uint8_t* data )
{
	memset( data, 0, DATA_SAGDP_SIZE );
	memset( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, 0xff, SAGDP_PACKETID_SIZE );
}

uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* nonce, uint16_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data, uint8_t* lsm )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
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

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = *buffIn & SAGDP_P_STATUS_FULL_MASK; // get "our" bits
	PRINTF( "handlerSAGDP_receiveUP(): state: %d, packet_status: %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			// TODO: process
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
//			PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
//			return SAGDP_RET_OK; // just ignore
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
				PRINTF( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;

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
					assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					return SAGDP_RET_OK; // ignored
				}
//				return handlerSAGDP_receiveRepeatedUP( timeout, sizeInOut, buffIn, buffOut, buffOutSize, stack, stackSize, data, lsm );
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply ) // above the range; silently ignore
			{
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK;
			}
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
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
				PRINTF( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					// re-send LSP
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;

					PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
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
			if ( !isreply ) // silently ignore
			{
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
				assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
				return SAGDP_RET_OK; // ignored
			}
		}
#endif
		else // allowed combination: packet_status == SAGDP_P_STATUS_FIRST in SAGDP_STATE_IDLE
		{
			assert( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST && state == SAGDP_STATE_IDLE );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			const uint8_t* this_chain_id = buffIn + 1;
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			bool is_resent = pid_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;

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
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			const uint8_t* this_chain_id = buffIn + 1;
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			bool is_resent = pid_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;

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
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
//				return SAGDP_RET_OK; // just ignore
				// TODO: form a packet
				return SAGDP_RET_TO_HIGHER_ERROR;
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
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, sizeInOut[0] & 3 );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply )
			{
				PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
//				return SAGDP_RET_OK; // silently ignore
				// TODO: form a packet
				return SAGDP_RET_TO_HIGHER_ERROR;
			}
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
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
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			// main question: is it a re-sent or a start of an actually new chain 
			bool current = pid_compare( buffIn + 1, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET ) == 0;
			if ( current )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;

				PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

				*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
				memcpy( buffOut, lsm, *sizeInOut );
				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
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
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", buffIn[1], buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = pid_compare( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
				if ( ( buffIn[0] & SAGDP_P_STATUS_NO_RESEND ) == 0 )
				{
					PRINTF( "SAGDP: state = %d, packet_status = %d; isold\n", state, packet_status );
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;

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
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, sizeInOut[0] & 3 );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( buffIn + 1, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply ) // silently ignore
			{
				// send an error message to a communication partner and reinitialize
				buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
				*sizeInOut = 1;
				// TODO: add other relevant data, if any, and update sizeInOut
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
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
		// send an error message to a communication partner and reinitialize
		buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
		*sizeInOut = 1;
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

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	if ( state == SAGDP_STATE_IDLE )
	{
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
		*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
		memcpy( buffOut, lsm, *sizeInOut );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
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

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = buffIn[0];
	PRINTF( "handlerSAGDP_receiveHLP(): state = %d, packet_status = %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
		if ( ( packet_status & SAGDP_P_STATUS_FIRST ) == 0 || ( packet_status & SAGDP_P_STATUS_TERMINATING ) )
		{
			// send an error message to a communication partner and reinitialize
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}
		assert( ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_FIRST ); // in idle state we can expect only "first" packet

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		memcpy( data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		memcpy( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

		// form a UP packet
		uint8_t* writeptr = buffOut;
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		*(writeptr++) = ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );
//		uint8_t* pid = data + DATA_SAGDP_LRECEIVED_PID_OFFSET;
//		PRINTF( "handlerSAGDP_receiveHLP(): PID reply-to (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		if ( packet_status == SAGDP_P_STATUS_FIRST )
			memcpy( writeptr, nonce, SAGDP_LRECEIVED_PID_SIZE );
		else
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
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
//		*(data+DATA_SAGDP_ALREADY_REPLIED_OFFSET) = 0;
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		if ( packet_status & SAGDP_P_STATUS_FIRST )
		{
			// send an error message to a communication partner and reinitialize
			buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
			*sizeInOut = 1;
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			return SAGDP_RET_SYS_CORRUPTED;
		}

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		PRINTF( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
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
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else // invalid states
	{
		// send an error message to a communication partner and reinitialize
		buffOut[0] = SAGDP_P_STATUS_ERROR_MSG;
		*sizeInOut = 1;
		// TODO: add other relevant data, if any, and update sizeInOut
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}
