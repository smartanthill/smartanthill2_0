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






void sagdp_init( uint8_t* data )
{
	memset( data, 0, DATA_SAGDP_SIZE );
//	memset( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, 0xff, SAGDP_PACKETID_SIZE );
}

uint8_t handlerSAGDP_timer( uint8_t* timeout, uint8_t* nonce, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data )
{
	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		INCREMENT_COUNTER( 20, "handlerSAGDP_timer(), packet resent" );
		assert( nonce != NULL );
		memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );
		cappedExponentiateLTO( data + DATA_SAGDP_LTO_OFFSET );
		*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
//		uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//		zepto_write_block( mem_h, lsm, packet_sz );
		zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE;
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // other states: ignore
	{
		return SAGDP_RET_OK;
	}
}

uint8_t handlerSAGDP_receiveUP( uint8_t* timeout, uint8_t* nonce, uint8_t* pid, REQUEST_REPLY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data )
{
	PRINTF( "handlerSAGDP_receiveNewUP():           pid: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
	// sizeInOut represents a size of UP packet
	// A new packet can come either in idle (beginning of a chain), or in wait-remote (continuation of a chain) state.
	// As a result SAGDP changes its state to wait-local, or (in case of errors) to not-initialized state

	INCREMENT_COUNTER( 21, "handlerSAGDP_receiveUP()" );

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = zepto_parse_uint8( &po );
	packet_status &= SAGDP_P_STATUS_FULL_MASK; // TODO: use bit-field processing instead
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

			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}


/*			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			if ( *sizeInOut > 1 + SAGDP_LRECEIVED_PID_SIZE )
			{
				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );
			}
			else
				*sizeInOut = 1;*/



			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
//			uint8_t* enc_reply_to = stack;
//			zepto_parse_read_block( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );

			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = pid_compare( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{ 
				// TODO: check against previous range
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
//					uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//					zepto_write_block( mem_h, lsm, packet_sz );
					zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
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
			bool isreply = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
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
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
//			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
//			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
//			return SAGDP_RET_SYS_CORRUPTED;
//			uint8_t* enc_reply_to = stack;
//			zepto_parse_read_block( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );

			uint8_t* pidprevlsent_first = data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = (!is_pid_zero(data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET)) && pid_compare( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0  && pid_compare( enc_reply_to, data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET ) >= 0;
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

//					uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//					zepto_write_block( mem_h, lsm, packet_sz );
					zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 25, "handlerSAGDP_receiveUP(), idle, is-old terminating" );
					assert( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					// send an error message to a communication partner and reinitialize
					zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
					// TODO: add other relevant data, if any, and update sizeInOut
					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
					PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_SYS_CORRUPTED;
				}
//				return handlerSAGDP_receiveRepeatedUP( timeout, sizeInOut, buffIn, buffOut, buffOutSize, stack, stackSize, data, lsm );
			}
			bool isreply = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply )
			{
//				if ( nonce == NULL )
//					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 26, "handlerSAGDP_receiveUP(), idle, too old, sys corrupted" );
				// send an error message to a communication partner and reinitialize
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
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
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
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
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
			uint8_t* this_chain_id = stack;
			zepto_parser_decode_uint( &po, this_chain_id, SAGDP_LSENT_PID_SIZE );
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): this_chain_id: %x%x%x%x%x%x\n", this_chain_id[0], this_chain_id[1], this_chain_id[2], this_chain_id[3], this_chain_id[4], this_chain_id[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): prev_chain_id: %x%x%x%x%x%x\n", prev_chain_id[0], prev_chain_id[1], prev_chain_id[2], prev_chain_id[3], prev_chain_id[4], prev_chain_id[5] );
			PRINTF( "handlerSAGDP_receiveNewUP():           pid: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
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
//				uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//				zepto_write_block( mem_h, lsm, packet_sz );
				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
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

/*				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );*/
				parser_obj po1;
				zepto_parser_init( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

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
/*			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			if ( *sizeInOut > 1 + SAGDP_LRECEIVED_PID_SIZE )
			{
				*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
				memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut-1 );
			}
			else
				*sizeInOut = 1;*/
			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
			uint8_t* this_chain_id = stack;
			zepto_parser_decode_uint( &po, this_chain_id, SAGDP_LSENT_PID_SIZE );
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
//				uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//				zepto_write_block( mem_h, lsm, packet_sz );
				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
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
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
//			uint8_t* enc_reply_to = stack;
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
//			bool isreply = memcmp( enc_reply_to + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			bool isold = pid_compare( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0;
			if ( isold )
			{
				// TODO: check against too-old status (previous last sent first)
//				return handlerSAGDP_receiveRepeatedUP( timeout, sizeInOut, buffIn, buffOut, buffOutSize, stack, stackSize, data, lsm );
				parser_obj po1;
				zepto_parser_init( &po1, mem_h );
				if ( ( zepto_parse_uint8( &po1 ) & SAGDP_P_STATUS_NO_RESEND ) == 0 )  // TODO: use bit-field processing instead
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


/*					*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					buffOut[0] |= SAGDP_P_STATUS_NO_RESEND;*/
					// TODO: construction below represents the logic above but is a little bit strange... think about proper processing

/*					uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					zepto_write_block( mem_h, lsm+1, packet_sz-1 );
					zepto_write_prepend_byte( mem_h, lsm[0] | SAGDP_P_STATUS_NO_RESEND );*/
					parser_obj po_lsm, po_lsm1;
					zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parser_init( &po_lsm1, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parse_skip_block( &po_lsm1, zepto_parsing_remaining_bytes( &po_lsm ) );
					uint8_t first_byte_of_lsm = zepto_parse_uint8( &po_lsm );
					first_byte_of_lsm |= SAGDP_P_STATUS_NO_RESEND;
					zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, &po_lsm, &po_lsm1, mem_h );
					zepto_write_prepend_byte( mem_h, first_byte_of_lsm );


					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend NOT requested" );
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
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
/*			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );*/
			parser_obj po1;
			zepto_parser_init( &po1, &po );
			uint16_t body_size = zepto_parsing_remaining_bytes( &po );
			zepto_parse_skip_block( &po1, body_size );
			zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
			zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

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
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// main question: is it a re-sent or a start of an actually new chain 
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
//			uint8_t* enc_reply_to = stack;
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			bool current = pid_compare( enc_reply_to, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET ) == 0;
			if ( current )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 42, "handlerSAGDP_receiveUP(), wait-remote, first, resent" );

				PRINTF( "handlerSAGDP_receiveUP(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				memcpy( data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET, nonce, SAGDP_LSENT_PID_SIZE );

//				uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
//				zepto_write_block( mem_h, lsm, packet_sz );
				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
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

//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
//			uint8_t* enc_reply_to = stack;
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			uint8_t* pidprevlsent_first = data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = (!is_pid_zero(data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET)) && pid_compare( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET ) < 0  && pid_compare( enc_reply_to, data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET ) >= 0;
			if ( isold )
			{
				parser_obj po1;
				zepto_parser_init( &po1, mem_h );
				if ( ( zepto_parse_uint8( &po1 ) & SAGDP_P_STATUS_NO_RESEND ) == 0 )  // TODO: use bit-field processing instead
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
					/*sizeInOut = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					memcpy( buffOut, lsm, *sizeInOut );
					buffOut[0] |= SAGDP_P_STATUS_NO_RESEND;*/
					// TODO: construction below represents the logic above but is a little bit strange... think about proper 

/*					uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
					zepto_write_block( mem_h, lsm+1, packet_sz-1 );
					zepto_write_prepend_byte( mem_h, lsm[0] | SAGDP_P_STATUS_NO_RESEND );*/
					parser_obj po_lsm, po_lsm1;
					zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parser_init( &po_lsm1, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parse_skip_block( &po_lsm1, zepto_parsing_remaining_bytes( &po_lsm ) );
					uint8_t first_byte_of_lsm = zepto_parse_uint8( &po_lsm );
					first_byte_of_lsm |= SAGDP_P_STATUS_NO_RESEND;
					zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, &po_lsm, &po_lsm1, mem_h );
					zepto_write_prepend_byte( mem_h, first_byte_of_lsm );

					*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handlerSAGDP_receiveUP(), wait-remote, is-old, resend NOT requested" );
					PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( !isreply ) // silently ignore
			{
				// send an error message to a communication partner and reinitialize
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 46, "handlerSAGDP_receiveUP(), wait-remote, !is-reply, ignored" );
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
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
/*			*sizeInOut -= SAGDP_LRECEIVED_PID_SIZE;
			buffOut[0] = ( packet_status & SAGDP_P_STATUS_MASK );
			memcpy( buffOut + 1, buffIn + 1 + SAGDP_LRECEIVED_PID_SIZE, *sizeInOut - 1 );*/
			parser_obj po1;
			zepto_parser_init( &po1, &po );
			uint16_t body_size = zepto_parsing_remaining_bytes( &po );
			zepto_parse_skip_block( &po1, body_size );
			zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
			zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			*( data + DATA_SAGDP_STATE_OFFSET ) = ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#endif
	}

	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 40, "handlerSAGDP_receiveUP(), wait-remote, error" );
			cancelLTO( data + DATA_SAGDP_LTO_OFFSET );
			*timeout = *(data + DATA_SAGDP_LTO_OFFSET);
			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}

			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST ) // TODO: same plus request ACK
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
			uint8_t* this_chain_id = stack;
			zepto_parser_decode_uint( &po, this_chain_id, SAGDP_LSENT_PID_SIZE );
			const uint8_t* prev_chain_id = data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET;
			bool is_resent = pid_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
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
		else // TODO: make sure no important option is left
		{
			assert( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE || ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_TERMINATING );
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
//			uint8_t* enc_reply_to = stack;
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
//			bool isreply = memcmp( enc_reply_to + 1, data + DATA_SAGDP_LSENT_PID_OFFSET, SAGDP_LSENT_PID_SIZE ) == 0;
			bool isrepeated = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( isrepeated ) // so we've got what we are currently processing
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
			}
			else // ignore
			{
				INCREMENT_COUNTER( 46, "handlerSAGDP_receiveUP(), wait-remote, !is-reply, ignored" );
				PRINTF( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK; // silently ignore
				// TODO: form a packet
//				return SAGDP_RET_TO_HIGHER_ERROR;
			}
		}
#else // USED_AS_MASTER not defined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 40, "handlerSAGDP_receiveUP(), wait-remote, error" );
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
			PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			assert(0);
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST ) // TODO: same plus request ACK
		{
			INCREMENT_COUNTER( 41, "handlerSAGDP_receiveUP(), wait-remote, first" );
			// main question: is it a re-sent or a start of an actually new chain 
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			bool current = pid_compare( enc_reply_to, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET ) == 0;
			if ( current )
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
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

//			zepto_parse_read_block( &po, stack, SAGDP_LSENT_PID_SIZE );
//			uint8_t* enc_reply_to = stack;
			uint8_t* enc_reply_to = stack;
			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			uint8_t* pidprevlsent_first = data + DATA_SAGDP_PREV_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_first = data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET;
			uint8_t* pidlsent_last = data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET;
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			PRINTF( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isrepeated = is_pid_in_range( enc_reply_to, data + DATA_SAGDP_FIRST_LSENT_PID_OFFSET, data + DATA_SAGDP_NEXT_LSENT_PID_OFFSET );
			if ( isrepeated ) // so we've got what we are currently processing
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
			}
			else // if not old, then unexpected
			{
				// send an error message to a communication partner and reinitialize
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 40, "handlerSAGDP_receiveUP(), wait-remote, error" );
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
				// TODO: add other relevant data, if any, and update sizeInOut
				*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
				PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
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
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
#endif
		// TODO: add other relevant data, if any, and update sizeInOut
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		PRINTF( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveRequestResendLSP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data )
{
	// SAGDP can legitimately receive a repeated packet in wait-remote state (the other side sounds like "we have not received anything from you; please resend, only then we will probably send you something new")
	// LSP must be resent

//	uint16_t packet_sz = *(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET);
	parser_obj po_lsm;
	zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
	if ( zepto_parsing_remaining_bytes( &po_lsm ) == 0 )
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
//		zepto_write_block( mem_h, lsm, packet_sz );
		zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
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
//		zepto_write_block( mem_h, lsm, packet_sz );
		zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_IDLE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		INCREMENT_COUNTER( 62, "handlerSAGDP_receiveRequestResendLSP(), invalid state" );
		// send an error message to a communication partner and reinitialize
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
		// TODO: add other relevant data, if any, and update sizeInOut
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handlerSAGDP_receiveHLP( uint8_t* timeout, uint8_t* nonce, MEMORY_HANDLE mem_h, uint8_t* stack, int stackSize, uint8_t* data )
{
	// Important: sizeInOut is a size of the message; returned size: sizeInOut += SAGDP_LRECEIVED_PID_SIZE
	//
	// there are two states when SAGDP can legitimately receive a packet from a higher level: idle (packet is first in the chain), and wait-local (any subsequent packet)
	// the packet is processed and passed for further sending; SAGDP waits for its PID and thus transits to SAGDP_STATE_WAIT_PID
	//
	// It is a responsibility of a higher level to report the status of a packet. 
	//

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	INCREMENT_COUNTER( 70, "handlerSAGDP_receiveHLP()" );

	uint8_t state = *( data + DATA_SAGDP_STATE_OFFSET );
	uint8_t packet_status = zepto_parse_uint8( &po );
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

			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
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
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		parser_obj po1;
		zepto_parser_init( &po1, &po );
		uint16_t body_size = zepto_parsing_remaining_bytes( &po );
		zepto_parse_skip_block( &po1, body_size );
		zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
//		zepto_write_prepend_block( mem_h, data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		uint8_t* enc_pid_end = stack;
		zepto_parser_encode_uint( data + DATA_SAGDP_LRECEIVED_CHAIN_ID_OFFSET, SAGDP_LRECEIVED_PID_SIZE, &enc_pid_end );
		zepto_write_prepend_block( mem_h, stack, enc_pid_end - stack );
		zepto_write_prepend_byte( mem_h, packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );

		// save a copy
//		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = zepto_writer_get_response_size( mem_h );
//		zepto_writer_get_copy_of_response( mem_h, lsm );
		zepto_copy_response_to_response_of_another_handle( mem_h, MEMORY_HANDLE_SAGDP_LSM );
		zepto_response_to_request( MEMORY_HANDLE_SAGDP_LSM );

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
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
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
		assert( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		parser_obj po1;
		zepto_parser_init( &po1, &po );
		uint16_t body_size = zepto_parsing_remaining_bytes( &po );
		zepto_parse_skip_block( &po1, body_size );
		zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
//		zepto_write_prepend_block( mem_h, data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE );
		uint8_t* enc_pid_end = stack;
		zepto_parser_encode_uint( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, SAGDP_LRECEIVED_PID_SIZE, &enc_pid_end );
		zepto_write_prepend_block( mem_h, stack, enc_pid_end - stack );
		zepto_write_prepend_byte( mem_h, packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );

		// save a copy
//		*(uint16_t*)(data+DATA_SAGDP_LSM_SIZE_OFFSET) = zepto_writer_get_response_size( mem_h );
//		zepto_writer_get_copy_of_response( mem_h, lsm );
		zepto_copy_response_to_response_of_another_handle( mem_h, MEMORY_HANDLE_SAGDP_LSM );
		zepto_response_to_request( MEMORY_HANDLE_SAGDP_LSM );

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
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
		// TODO: add other relevant data, if any, and update sizeInOut
#endif
		*( data + DATA_SAGDP_STATE_OFFSET ) = SAGDP_STATE_NOT_INITIALIZED;
		INCREMENT_COUNTER( 76, "handlerSAGDP_receiveHLP(), invalid state" );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

