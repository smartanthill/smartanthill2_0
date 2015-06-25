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

static SAGDP_DATA sagdp_data;

// SAGDP timer constants
// TODO: revise when values are finalized in the documentation
///#define SAGDP_LTO_TO_SEC( x ) x // just 1 unit == 1 sec
#define SAGDP_LTO_START 3 // 3 sec so far
#define SAGDP_LTO_MAX 189
#define SAGDP_LTO_POW_MAX 11


void setIniLTO( uint8_t* lto )
{
	*lto = SAGDP_LTO_START;
}

void cappedExponentiateLTO( uint8_t* lto )
{
	if ( *lto >= SAGDP_LTO_MAX ) return;
	uint16_t _lto = *lto;
	_lto *= 3;
	_lto /= 2;
	*lto = (uint8_t)_lto;
}

void cancelLTO( uint8_t* lto )
{
	*lto = 0xFF;
}

bool is_pid_in_range( const sa_uint48_t pid, const sa_uint48_t first_pid, const sa_uint48_t last_pid )
{
	return sa_uint48_compare( pid, first_pid ) >=0 && sa_uint48_compare( pid, last_pid ) <= 0;
}

bool is_pid_zero( const sa_uint48_t pid )
{
	return is_uint48_zero( pid );
}






void sagdp_init( /*SAGDP_DATA* sagdp_data*/ )
{
	sagdp_data.state = SAGDP_STATE_IDLE;
	cancelLTO( &(sagdp_data.last_timeout) );
	sa_uint48_set_zero( sagdp_data.last_received_chain_id );
	sa_uint48_set_zero( sagdp_data.last_received_packet_id );
	sa_uint48_set_zero( sagdp_data.first_last_sent_packet_id );
	sa_uint48_set_zero( sagdp_data.next_last_sent_packet_id );
	sa_uint48_set_zero( sagdp_data.prev_first_last_sent_packet_id );
	sagdp_data.resent_ordinal = 0;
	sagdp_data.event_type = SAGDP_EV_NONE; // in this case we do not care about next_event_time
	ZEPTO_DEBUG_PRINTF_1( "------------ event set to SAGDP_EV_NONE ----------------\n" );
}

// PROCESSING TIMEOUT-BASED SEQUENCES

#define SAGDP_LTO_INCREMENT_BY_CAPPED_EXP_SEC( tval, sec_pow ) \
	{\
		sa_time_val tmp_t; \
		cappedExponentiateLTO( &(sagdp_data.last_timeout) ); \
		if (sec_pow > SAGDP_LTO_POW_MAX) sec_pow = SAGDP_LTO_POW_MAX; \
		SA_TIME_LOAD_TICKS_FOR_1_SEC( tmp_t ); \
		SA_TIME_MUL_TICKS_BY_2( tmp_t ) \
		uint8_t i; for ( i=0; i<=sec_pow; i++) SA_TIME_MUL_TICKS_BY_1_AND_A_HALF( tmp_t ) \
		SA_TIME_INCREMENT_BY_TICKS( tval, tmp_t ) \
	}


#define SAGDP_INIT_RESENT_SEQUENCE \
	sagdp_data.resent_ordinal = 1; \
	sagdp_data.event_type = SAGDP_EV_RESEND_LSP; ZEPTO_DEBUG_PRINTF_1( "------------ event set to SAGDP_EV_RESEND_LSP ----------------\n" );\
	setIniLTO( &(sagdp_data.last_timeout) ); \
	SAGDP_LTO_INCREMENT_BY_CAPPED_EXP_SEC( *currt, sagdp_data.resent_ordinal ) \
	sa_hal_time_val_copy_from( &(sagdp_data.next_event_time), currt );


#define SAGDP_CANCEL_RESENT_SEQUENCE \
	sagdp_data.resent_ordinal = 0; \
	sagdp_data.event_type = SAGDP_EV_NONE; ZEPTO_DEBUG_PRINTF_1( "------------ event set to SAGDP_EV_NONE ----------------\n" ); \
	cancelLTO( &(sagdp_data.last_timeout) );


#define SAGDP_REGISTER_SUBSEQUENT_RESENT \
	(sagdp_data.resent_ordinal) ++; \
	sa_hal_time_val_copy_from(currt,  &(sagdp_data.next_event_time) );


#define SAGDP_REGISTER_SUBSEQUENT_RESENT_AND_RESET_TIMEOUT \
	(sagdp_data.resent_ordinal) ++; \
	ZEPTO_DEBUG_ASSERT( sagdp_data.last_timeout != 0 ); \
	SAGDP_LTO_INCREMENT_BY_CAPPED_EXP_SEC( *currt, sagdp_data.resent_ordinal ) \
	sa_hal_time_val_copy_from( &(sagdp_data.next_event_time), currt );


#define SAGDP_SHOULD_RESENT \
	( sagdp_data.event_type == SAGDP_EV_RESEND_LSP && sa_hal_time_val_is_less( &(sagdp_data.next_event_time), currt ) )


#define SAGDP_ASSERT_NO_SEQUENCE \
	ZEPTO_DEBUG_ASSERT( sagdp_data.event_type == SAGDP_EV_NONE );


#define SAGDP_ASSERT_SEQUENCE_IN_PROGRESS \
	ZEPTO_DEBUG_ASSERT( sagdp_data.resent_ordinal != 0 && sagdp_data.event_type == SAGDP_EV_RESEND_LSP );



uint8_t handler_sagdp_timer( sa_time_val* currt, waiting_for* wf, sasp_nonce_type nonce, REQUEST_REPLY_HANDLE mem_h/*, SAGDP_DATA* sagdp_data*/ )
{
	uint8_t state = sagdp_data.state;
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		INCREMENT_COUNTER( 20, "handlerSAGDP_timer(), packet resent" );

		if ( SAGDP_SHOULD_RESENT )
		{
/*			zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
			ZEPTO_DEBUG_ASSERT( sagdp_data.state == SAGDP_STATE_WAIT_REMOTE );
			return SAGDP_RET_TO_LOWER_REPEATED;*/
			parser_obj po_lsm;
			zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
			ZEPTO_DEBUG_ASSERT ( zepto_parsing_remaining_bytes( &po_lsm ) != 0 );

			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 60, "handler_sagdp_timer(), wait-remote" );

			SAGDP_REGISTER_SUBSEQUENT_RESENT_AND_RESET_TIMEOUT

			ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_timer(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

			// apply nonce
			sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

			zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
			ZEPTO_DEBUG_ASSERT( sagdp_data.state = SAGDP_STATE_WAIT_REMOTE );
			return SAGDP_RET_TO_LOWER_REPEATED;
		}
		else
			return SAGDP_RET_OK;
	}
	else // other states: ignore
	{
		SAGDP_ASSERT_NO_SEQUENCE;
		return SAGDP_RET_OK;
	}
}

uint8_t handler_sagdp_receive_up( sa_time_val* currt, waiting_for* wf, sasp_nonce_type nonce, uint8_t* pid, REQUEST_REPLY_HANDLE mem_h/*, SAGDP_DATA* sagdp_data*/ )
{
	ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP():           pid: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );

	INCREMENT_COUNTER( 21, "handler_sagdp_receive_up()" );

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	uint8_t state = sagdp_data.state;
	uint8_t packet_status = zepto_parse_uint8( &po );
	packet_status &= SAGDP_P_STATUS_FULL_MASK; // TODO: use bit-field processing instead
	ZEPTO_DEBUG_PRINTF_3( "handler_sagdp_receive_up(): state: %d, packet_status: %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
		SAGDP_ASSERT_NO_SEQUENCE;

#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 22, "handler_sagdp_receive_up(), idle, error message" );

//			cancelLTO( &(sagdp_data.last_timeout) );
//			*timeout = sagdp_data.last_timeout;

			//+++ TODO: revise commented out logic below
/*			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init_by_parser( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else*/
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}

			sagdp_data.state = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
			sa_uint48_t enc_reply_to;
//			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );

			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = sa_uint48_compare( enc_reply_to, sagdp_data.first_last_sent_packet_id ) < 0;
			if ( isold )
			{ 
				// TODO: check against previous range
				INCREMENT_COUNTER( 23, "handler_sagdp_receive_up(), idle, is-old" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 24, "handler_sagdp_receive_up(), idle, is-old intermediate" );

					ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

					// re-send LSP
					zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
					// SAGDP status remains the same
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 25, "handler_sagdp_receive_up(), idle, is-old terminating" );
					ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					return SAGDP_RET_OK; // ignored
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( !isreply ) // above the range; silently ignore
			{
				INCREMENT_COUNTER( 26, "handler_sagdp_receive_up(), idle, too old, ignored" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK;
			}
			INCREMENT_COUNTER( 27, "handler_sagdp_receive_up(), idle, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				return SAGDP_RET_OK; // ignored
			}
			else
			{
				ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
				return SAGDP_RET_OK; // ignored
			}
		}
#else // USED_AS_MASTER not ndefined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG ) // unexpected at slave's side
		{
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 22, "handler_sagdp_receive_up(), idle, error message" );
			// send an error message to a communication partner and reinitialize
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
			ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			SAGDP_CANCEL_RESENT_SEQUENCE
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST )
		{
			sasp_nonce_type enc_reply_to;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );

#ifdef SA_DEBUG
			uint8_t* pidprevlsent_first = sagdp_data.prev_first_last_sent_packet_id;
			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
#endif

			bool isold = (!is_pid_zero(sagdp_data.prev_first_last_sent_packet_id)) && sa_uint48_compare( enc_reply_to, sagdp_data.first_last_sent_packet_id ) < 0  && sa_uint48_compare( enc_reply_to, sagdp_data.prev_first_last_sent_packet_id ) >= 0;
			if ( isold )
			{
				INCREMENT_COUNTER( 23, "handler_sagdp_receive_up(), idle, is-old" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP OK: state = %d, packet_status = %d; isold\n", state, packet_status );
				if ( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_INTERMEDIATE )
				{
					// re-send LSP
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 24, "handler_sagdp_receive_up(), idle, is-old intermediate" );

					ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

					zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
					SAGDP_REGISTER_SUBSEQUENT_RESENT
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 25, "handler_sagdp_receive_up(), idle, is-old terminating" );
					ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
					// send an error message to a communication partner and reinitialize
					zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
					// TODO: add other relevant data, if any, and update sizeInOut
					sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
					ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_SYS_CORRUPTED;
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( !isreply )
			{
				INCREMENT_COUNTER( 26, "handler_sagdp_receive_up(), idle, too old, sys corrupted" );
				// send an error message to a communication partner and reinitialize
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
				// TODO: add other relevant data, if any, and update sizeInOut
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 27, "handler_sagdp_receive_up(), idle, intermediate, sys corrupted" );
				// send an error message to a communication partner and reinitialize
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
				// TODO: add other relevant data, if any, and update sizeInOut
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			else
			{
				INCREMENT_COUNTER( 28, "handler_sagdp_receive_up(), idle, terminating, ignored" );
				ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) ==  SAGDP_P_STATUS_TERMINATING );
				SAGDP_CANCEL_RESENT_SEQUENCE
				return SAGDP_RET_OK; // ignored
			}
		}
#endif
		else // allowed combination: packet_status == SAGDP_P_STATUS_FIRST in SAGDP_STATE_IDLE
		{
			INCREMENT_COUNTER( 29, "handler_sagdp_receive_up(), idle, first" );
			ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST && state == SAGDP_STATE_IDLE );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			sasp_nonce_type this_chain_id;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, this_chain_id );
			const uint8_t* prev_chain_id = sagdp_data.last_received_chain_id;
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): this_chain_id: %x%x%x%x%x%x\n", this_chain_id[0], this_chain_id[1], this_chain_id[2], this_chain_id[3], this_chain_id[4], this_chain_id[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): prev_chain_id: %x%x%x%x%x%x\n", prev_chain_id[0], prev_chain_id[1], prev_chain_id[2], prev_chain_id[3], prev_chain_id[4], prev_chain_id[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP():           pid: %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
			bool is_resent = sa_uint48_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 30, "handler_sagdp_receive_up(), idle, first, resent" );

				ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

				// re-send LSP
				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
				//SAGDP_REGISTER_SUBSEQUENT_RESENT
				// SAGDP status remains the same
				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 31, "handler_sagdp_receive_up(), idle, first, new" );
				cancelLTO( &(sagdp_data.last_timeout) );
//				*timeout = sagdp_data.last_timeout;
				sa_uint48_init_by( sagdp_data.last_received_packet_id, pid );
				sa_uint48_init_by( sagdp_data.last_received_chain_id, this_chain_id );

				parser_obj po1;
				zepto_parser_init_by_parser( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

				sagdp_data.state = SAGDP_STATE_WAIT_LOCAL;
				return SAGDP_RET_TO_HIGHER;
			}
		}
	}

	else if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		SAGDP_ASSERT_SEQUENCE_IN_PROGRESS;

#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 40, "handler_sagdp_receive_up(), wait-remote, error" );
/*			cancelLTO( &(sagdp_data.last_timeout) );
			*timeout = sagdp_data.last_timeout;*/
			SAGDP_CANCEL_RESENT_SEQUENCE;
			//+++ TODO: revise commented out logic below
/*			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init_by_parser( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else*/
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}

			sagdp_data.state = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			SAGDP_CANCEL_RESENT_SEQUENCE;
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handler_sagdp_receive_up(), wait-remote, first" );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			sa_uint48_t this_chain_id;
//			zepto_parser_decode_uint( &po, this_chain_id, SAGDP_LSENT_PID_SIZE );
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, this_chain_id );
			const uint8_t* prev_chain_id = sagdp_data.last_received_chain_id;
			bool is_resent = sa_uint48_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;

				INCREMENT_COUNTER( 42, "handler_sagdp_receive_up(), wait-remote, first, resent" );
				ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

				// re-send LSP
				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
				// SAGDP status remains the same

				SAGDP_REGISTER_SUBSEQUENT_RESENT
				// TODO: think about going back to minimal timeout

				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 43, "handler_sagdp_receive_up(), wait-remote, first, new (ignored)" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_OK; // just ignore
			}
		}
		else
		{
			ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE || ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_TERMINATING );
			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			sa_uint48_t enc_reply_to;
//			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isold = sa_uint48_compare( enc_reply_to, sagdp_data.first_last_sent_packet_id ) < 0;
			if ( isold )
			{
				// TODO: check against too-old status (previous last sent first)
				parser_obj po1;
				zepto_parser_init( &po1, mem_h );
				if ( ( zepto_parse_uint8( &po1 ) & SAGDP_P_STATUS_NO_RESEND ) == 0 )  // TODO: use bit-field processing instead
				{
					ZEPTO_DEBUG_PRINTF_3( "SAGDP: state = %d, packet_status = %d; isold\n", state, packet_status );
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 44, "handler_sagdp_receive_up(), wait-remote, is-old, resend requested" );

					ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

//					cappedExponentiateLTO( &(sagdp_data.last_timeout) );
//					*timeout = sagdp_data.last_timeout;

					SAGDP_REGISTER_SUBSEQUENT_RESENT
					// TODO: think about going back to minimal timeout


					parser_obj po_lsm, po_lsm1;
					zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parser_init( &po_lsm1, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parse_skip_block( &po_lsm1, zepto_parsing_remaining_bytes( &po_lsm ) );
					uint8_t first_byte_of_lsm = zepto_parse_uint8( &po_lsm );
					first_byte_of_lsm |= SAGDP_P_STATUS_NO_RESEND;
					zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, &po_lsm, &po_lsm1, mem_h );
					zepto_write_prepend_byte( mem_h, first_byte_of_lsm );


					sagdp_data.state = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handler_sagdp_receive_up(), wait-remote, is-old, resend NOT requested" );
					ZEPTO_DEBUG_PRINTF_3( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( !isreply )
			{
				INCREMENT_COUNTER( 46, "handler_sagdp_receive_up(), wait-remote, !is-reply, ignored" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK; // silently ignore
			}
			INCREMENT_COUNTER( 47, "handler_sagdp_receive_up(), wait-remote, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				INCREMENT_COUNTER( 48, "handler_sagdp_receive_up(), wait-remote, other, intermediate" );
				ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_hlp(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
				sa_uint48_init_by( sagdp_data.last_received_packet_id, pid );
			}
			// form a packet for higher level
			parser_obj po1;
			zepto_parser_init_by_parser( &po1, &po );
			uint16_t body_size = zepto_parsing_remaining_bytes( &po );
			zepto_parse_skip_block( &po1, body_size );
			zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
			zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

//			cancelLTO( &(sagdp_data.last_timeout) );
//			*timeout = sagdp_data.last_timeout;
			SAGDP_CANCEL_RESENT_SEQUENCE;
			sagdp_data.state = ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#else // USED_AS_MASTER not defined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 40, "handler_sagdp_receive_up(), wait-remote, error" );
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
			ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			SAGDP_CANCEL_RESENT_SEQUENCE
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST )
		{
			INCREMENT_COUNTER( 41, "handler_sagdp_receive_up(), wait-remote, first" );
			// main question: is it a re-sent or a start of an actually new chain 
			sasp_nonce_type enc_reply_to;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );
			bool current = sa_uint48_compare( enc_reply_to, sagdp_data.last_received_chain_id ) == 0;
			if ( current )
			{
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 42, "handler_sagdp_receive_up(), wait-remote, first, resent" );

				ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

				// apply nonce
				sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

				zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );

				SAGDP_REGISTER_SUBSEQUENT_RESENT
				// TODO: think about going back to minimal timeout

				return SAGDP_RET_TO_LOWER_REPEATED;
			}
			else
			{
				INCREMENT_COUNTER( 43, "handler_sagdp_receive_up(), wait-remote, first, new (applied)" );
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				SAGDP_CANCEL_RESENT_SEQUENCE
				return SAGDP_RET_START_OVER_FIRST_RECEIVED;
			}
		}
		else // intermediate or terminating
		{
			ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST );

			sasp_nonce_type enc_reply_to;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );

#ifdef SA_DEBUG
			uint8_t* pidprevlsent_first = sagdp_data.prev_first_last_sent_packet_id;
			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
#endif

			bool isold = (!is_pid_zero(sagdp_data.prev_first_last_sent_packet_id)) && sa_uint48_compare( enc_reply_to, sagdp_data.first_last_sent_packet_id ) < 0  && sa_uint48_compare( enc_reply_to, sagdp_data.prev_first_last_sent_packet_id ) >= 0;
			if ( isold )
			{
				parser_obj po1;
				zepto_parser_init( &po1, mem_h );
				if ( ( zepto_parse_uint8( &po1 ) & SAGDP_P_STATUS_NO_RESEND ) == 0 )  // TODO: use bit-field processing instead
				{
					ZEPTO_DEBUG_PRINTF_3( "SAGDP: state = %d, packet_status = %d; isold\n", state, packet_status );
					if ( nonce == NULL )
						return SAGDP_RET_NEED_NONCE;
					INCREMENT_COUNTER( 44, "handler_sagdp_receive_up(), wait-remote, is-old, resend requested" );

					ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_up(): nonce: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

					// apply nonce
					sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

					cappedExponentiateLTO( &(sagdp_data.last_timeout) );
//					*timeout = sagdp_data.last_timeout;
					parser_obj po_lsm, po_lsm1;
					zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parser_init( &po_lsm1, MEMORY_HANDLE_SAGDP_LSM );
					zepto_parse_skip_block( &po_lsm1, zepto_parsing_remaining_bytes( &po_lsm ) );
					uint8_t first_byte_of_lsm = zepto_parse_uint8( &po_lsm );
					first_byte_of_lsm |= SAGDP_P_STATUS_NO_RESEND;
					zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, &po_lsm, &po_lsm1, mem_h );
					zepto_write_prepend_byte( mem_h, first_byte_of_lsm );

					SAGDP_REGISTER_SUBSEQUENT_RESENT
					// TODO: think about going back to minimal timeout

					sagdp_data.state = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
					return SAGDP_RET_TO_LOWER_REPEATED;
				}
				else
				{
					INCREMENT_COUNTER( 45, "handler_sagdp_receive_up(), wait-remote, is-old, resend NOT requested" );
					ZEPTO_DEBUG_PRINTF_3( "SAGDP OK (no resend): state = %d, packet_status = %d\n", state, packet_status );
//					SAGDP_CANCEL_RESENT_SEQUENCE
					return SAGDP_RET_OK;
				}
			}
			bool isreply = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( !isreply ) // silently ignore
			{
				// send an error message to a communication partner and reinitialize
				if ( nonce == NULL )
					return SAGDP_RET_NEED_NONCE;
				INCREMENT_COUNTER( 46, "handler_sagdp_receive_up(), wait-remote, !is-reply, ignored" );
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
				// TODO: add other relevant data, if any, and update sizeInOut
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
			INCREMENT_COUNTER( 47, "handler_sagdp_receive_up(), wait-remote, other" );
			// for non-terminating, save packet ID
			if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE )
			{
				INCREMENT_COUNTER( 48, "handler_sagdp_receive_up(), wait-remote, other, intermediate" );
				ZEPTO_DEBUG_PRINTF_7( "handler_sagdp_receive_hlp(): PID of packet (LRECEIVED): %x%x%x%x%x%x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
//				memcpy( data + DATA_SAGDP_LRECEIVED_PID_OFFSET, pid, SAGDP_LRECEIVED_PID_SIZE );
				sa_uint48_init_by( sagdp_data.last_received_packet_id, pid );
			}
			// form a packet for higher level
			parser_obj po1;
			zepto_parser_init_by_parser( &po1, &po );
			uint16_t body_size = zepto_parsing_remaining_bytes( &po );
			zepto_parse_skip_block( &po1, body_size );
			zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
			zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );

//			cancelLTO( &(sagdp_data.last_timeout) );
			SAGDP_CANCEL_RESENT_SEQUENCE
//			*timeout = sagdp_data.last_timeout;
			sagdp_data.state = ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE ? SAGDP_STATE_WAIT_LOCAL : SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
#endif
	}

	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		SAGDP_ASSERT_NO_SEQUENCE;

#ifdef USED_AS_MASTER
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			INCREMENT_COUNTER( 40, "handler_sagdp_receive_up(), wait-remote, error" );
//			cancelLTO( &(sagdp_data.last_timeout) );
//			*timeout = sagdp_data.last_timeout;
			//+++ TODO: revise commented out logic below
/*			if ( zepto_parse_skip_block( &po, SAGDP_LRECEIVED_PID_SIZE ) )
			{
				parser_obj po1;
				zepto_parser_init_by_parser( &po1, &po );
				uint16_t body_size = zepto_parsing_remaining_bytes( &po );
				zepto_parse_skip_block( &po1, body_size );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_write_prepend_byte( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}
			else*/
			{
				zepto_write_uint8( mem_h, packet_status & SAGDP_P_STATUS_MASK );
			}

			sagdp_data.state = SAGDP_STATE_IDLE;
			return SAGDP_RET_TO_HIGHER;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST ) // TODO: same plus request ACK
		{
			INCREMENT_COUNTER( 41, "handler_sagdp_receive_up(), wait-remote, first" );
			// note: this "first" packet can be start of a new chain, or a re-sent of the beginning of the previous chain (if that previous chain had a length of 2)
			sa_uint48_t this_chain_id;
//			zepto_parser_decode_uint( &po, this_chain_id, SAGDP_LSENT_PID_SIZE );
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, this_chain_id );
			const uint8_t* prev_chain_id = sagdp_data.last_received_chain_id;
			bool is_resent = sa_uint48_compare( this_chain_id, prev_chain_id ) == 0;
			if ( is_resent )
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
					ZEPTO_DEBUG_ASSERT(0);
					return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
			}
			else
			{
				INCREMENT_COUNTER( 43, "handler_sagdp_receive_up(), wait-remote, first, new (ignored)" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_OK; // just ignore
			}
		}
		else // TODO: make sure no important option is left
		{
			ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_INTERMEDIATE || ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_TERMINATING );
			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			sa_uint48_t enc_reply_to;
//			zepto_parser_decode_uint( &po, enc_reply_to, SAGDP_LSENT_PID_SIZE );
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
			bool isrepeated = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( isrepeated ) // so we've got what we are currently processing
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
					ZEPTO_DEBUG_ASSERT(0);
					return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
			}
			else // ignore
			{
				INCREMENT_COUNTER( 46, "handler_sagdp_receive_up(), wait-remote, !is-reply, ignored" );
				ZEPTO_DEBUG_PRINTF_3( "SAGDP OK: CORRRUPTED: state = %d, packet_status = %d, !isreply\n", state, packet_status );
				return SAGDP_RET_OK; // silently ignore
			}
		}
#else // USED_AS_MASTER not defined
		if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			INCREMENT_COUNTER( 40, "handler_sagdp_receive_up(), wait-remote, error" );
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any, and update sizeInOut
			sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
			ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_IS_ACK )
		{
			SAGDP_CANCEL_RESENT_SEQUENCE
			ZEPTO_DEBUG_ASSERT(0);
			return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
		}
		else if ( ( packet_status & SAGDP_P_STATUS_MASK ) == SAGDP_P_STATUS_FIRST ) // TODO: same plus request ACK
		{
			INCREMENT_COUNTER( 41, "handler_sagdp_receive_up(), wait-remote, first" );
			// main question: is it a re-sent or a start of an actually new chain 
			sasp_nonce_type enc_reply_to;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );
			bool current = sa_uint48_compare( enc_reply_to, sagdp_data.last_received_chain_id ) == 0;
			if ( current )
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
					ZEPTO_DEBUG_ASSERT(0);
					return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
				}
				else
				{
					// already in work; ignore
					return SAGDP_RET_OK;
				}
			}
			else
			{
				INCREMENT_COUNTER( 43, "handler_sagdp_receive_up(), wait-remote, first, new (applied)" );
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_START_OVER_FIRST_RECEIVED;
			}
		}
		else // intermediate or terminating
		{
			ZEPTO_DEBUG_ASSERT( ( packet_status & SAGDP_P_STATUS_MASK ) != SAGDP_P_STATUS_FIRST );

			sasp_nonce_type enc_reply_to;
			zepto_parser_decode_encoded_uint_as_sa_uint48( &po, enc_reply_to );

#ifdef SA_DEBUG
			uint8_t* pidprevlsent_first = sagdp_data.prev_first_last_sent_packet_id;
			uint8_t* pidlsent_first = sagdp_data.first_last_sent_packet_id;
			uint8_t* pidlsent_last = sagdp_data.next_last_sent_packet_id;
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidprevlsent_first[0], pidprevlsent_first[1], pidprevlsent_first[2], pidprevlsent_first[3], pidprevlsent_first[4], pidprevlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent first   : %x%x%x%x%x%x\n", pidlsent_first[0], pidlsent_first[1], pidlsent_first[2], pidlsent_first[3], pidlsent_first[4], pidlsent_first[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID reply-to in packet: %x%x%x%x%x%x\n", enc_reply_to[0], enc_reply_to[1], enc_reply_to[2], enc_reply_to[3], enc_reply_to[4], enc_reply_to[5] );
			ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receiveNewUP(): PID last sent last    : %x%x%x%x%x%x\n", pidlsent_last[0], pidlsent_last[1], pidlsent_last[2], pidlsent_last[3], pidlsent_last[4], pidlsent_last[5] );
#endif

			bool isrepeated = is_pid_in_range( enc_reply_to, sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id );
			if ( isrepeated ) // so we've got what we are currently processing
			{
				bool ack_rq = false; // TODO: implement this branch
				if ( ack_rq )
				{
					// if ACK is requested, send it
					ZEPTO_DEBUG_ASSERT(0);
					return SAGDP_RET_SYS_CORRUPTED; // just as long as not implemented
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
				INCREMENT_COUNTER( 40, "handler_sagdp_receive_up(), wait-remote, error" );
				zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
				// TODO: add other relevant data, if any, and update sizeInOut
				sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
				ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
				return SAGDP_RET_SYS_CORRUPTED;
			}
		}
#endif
	}
	
	else // invalid states
	{
		INCREMENT_COUNTER( 50, "handler_sagdp_receive_up(), invalid state" );
#if !defined USED_AS_MASTER
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		// send an error message to a communication partner and reinitialize
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
#endif
		// TODO: add other relevant data, if any, and update sizeInOut
		sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
		ZEPTO_DEBUG_PRINTF_3( "SAGDP: CORRRUPTED: state = %d, packet_status = %d\n", state, packet_status );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handler_sagdp_receive_request_resend_lsp( sa_time_val* currt, waiting_for* wf, sasp_nonce_type nonce, MEMORY_HANDLE mem_h/*, SAGDP_DATA* sagdp_data*/ )
{
	// SAGDP can legitimately receive a repeated packet in wait-remote state (the other side sounds like "we have not received anything from you; please resend, only then we will probably send you something new")
	// LSP must be resent

	parser_obj po_lsm;
	zepto_parser_init( &po_lsm, MEMORY_HANDLE_SAGDP_LSM );
	if ( zepto_parsing_remaining_bytes( &po_lsm ) == 0 )
	{
		ZEPTO_DEBUG_ASSERT( sagdp_data.resent_ordinal == 0 );
		ZEPTO_DEBUG_ASSERT( sagdp_data.event_type == SAGDP_EV_NONE );
		INCREMENT_COUNTER( 63, "handler_sagdp_receive_request_resend_lsp(), no lsm" );
		return SAGDP_RET_TO_LOWER_NONE;
	}

	uint8_t state = sagdp_data.state;
	if ( state == SAGDP_STATE_WAIT_REMOTE )
	{
		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		INCREMENT_COUNTER( 60, "handler_sagdp_receive_request_resend_lsp(), wait-remote" );

		ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

//		cappedExponentiateLTO( &(sagdp_data.last_timeout) );
		SAGDP_CANCEL_RESENT_SEQUENCE
/*		SAGDP_LTO_INCREMENT_BY_CAPPED_EXP_SEC( *tval, sagdp_data.resent_ordinal );
		(sagdp_data.resent_ordinal)++;*/
		SAGDP_INIT_RESENT_SEQUENCE
//		sa_hal_time_val_copy_from( &(sagdp_data.next_event_time), tval );

//		*timeout = sagdp_data.last_timeout;
		zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
		sagdp_data.state = SAGDP_STATE_WAIT_REMOTE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	if ( state == SAGDP_STATE_IDLE )
	{
		SAGDP_ASSERT_NO_SEQUENCE;

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;
		INCREMENT_COUNTER( 61, "handler_sagdp_receive_request_resend_lsp(), idle" );

		ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

//		cappedExponentiateLTO( &(sagdp_data.last_timeout) );
//		SAGDP_CANCEL_RESENT_SEQUENCE
/*		SAGDP_LTO_INCREMENT_BY_CAPPED_EXP_SEC( *tval, sagdp_data.resent_ordinal );
		(sagdp_data.resent_ordinal)++;*/
//		SAGDP_INIT_RESENT_SEQUENCE

//		*timeout = sagdp_data.last_timeout;
		zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE_SAGDP_LSM, mem_h );
		sagdp_data.state = SAGDP_STATE_IDLE; // note that PID can be changed!
		return SAGDP_RET_TO_LOWER_REPEATED;
	}
	else // invalid states
	{
		INCREMENT_COUNTER( 62, "handler_sagdp_receive_request_resend_lsp(), invalid state" );
		// send an error message to a communication partner and reinitialize
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
		// TODO: add other relevant data, if any, and update sizeInOut
		sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

uint8_t handler_sagdp_receive_hlp( sa_time_val* currt, waiting_for* wf, sasp_nonce_type nonce, MEMORY_HANDLE mem_h/*, SAGDP_DATA* sagdp_data*/ )
{
	// It is a responsibility of a higher level to report the status of a packet. 
	//

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	INCREMENT_COUNTER( 70, "handler_sagdp_receive_hlp()" );

	uint8_t state = sagdp_data.state;
	uint8_t packet_status = zepto_parse_uint8( &po );
	ZEPTO_DEBUG_PRINTF_3( "handler_sagdp_receive_hlp(): state = %d, packet_status = %d\n", state, packet_status );

	if ( state == SAGDP_STATE_IDLE )
	{
		SAGDP_ASSERT_NO_SEQUENCE;

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
			sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
#endif // USED_AS_MASTER
			INCREMENT_COUNTER( 71, "handler_sagdp_receive_hlp(), idle, state/packet mismatch" );
			return SAGDP_RET_SYS_CORRUPTED;
		}
		ZEPTO_DEBUG_ASSERT( ( packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_FIRST ); // in idle state we can expect only "first" packet
		ZEPTO_DEBUG_ASSERT( packet_status == SAGDP_P_STATUS_FIRST );

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		sa_uint48_init_by( sagdp_data.prev_first_last_sent_packet_id, sagdp_data.first_last_sent_packet_id );
		sa_uint48_init_by( sagdp_data.first_last_sent_packet_id, nonce );
		sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );
		// "chain id" is shared between devices and therefore, should be unique for both sides, that is, shoud have master/slave distinguishing bit
		sa_uint48_init_by( sagdp_data.last_received_chain_id, nonce );
		//+++ TODO: next line and related MUST be re-thought and re-designed!!!!
		*(sagdp_data.last_received_chain_id + SASP_NONCE_TYPE_SIZE - 1) |= ( MASTER_SLAVE_BIT << 7 ); //!!!TODO: use bit field procesing instead; also: make sure this operation is safe

		// form a UP packet
		ZEPTO_DEBUG_ASSERT( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		parser_obj po1;
		zepto_parser_init_by_parser( &po1, &po );
		uint16_t body_size = zepto_parsing_remaining_bytes( &po );
		zepto_parse_skip_block( &po1, body_size );
		zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
		zepto_parser_encode_and_prepend_sa_uint48( mem_h, sagdp_data.last_received_chain_id );
		zepto_write_prepend_byte( mem_h, packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );

		// save a copy
		zepto_copy_response_to_response_of_another_handle( mem_h, MEMORY_HANDLE_SAGDP_LSM );
		zepto_response_to_request( MEMORY_HANDLE_SAGDP_LSM );

		// request set timer
		SAGDP_INIT_RESENT_SEQUENCE

		sagdp_data.state = SAGDP_STATE_WAIT_REMOTE;
		INCREMENT_COUNTER( 72, "handler_sagdp_receive_hlp(), idle, PACKET=FIRST" );
		return SAGDP_RET_TO_LOWER_NEW;
	}
	else if ( state == SAGDP_STATE_WAIT_LOCAL )
	{
		SAGDP_ASSERT_NO_SEQUENCE;

		if ( packet_status & SAGDP_P_STATUS_FIRST )
		{
#ifdef USED_AS_MASTER
			// TODO: should we do anything else but error reporting?
#else
			// send an error message to a communication partner and reinitialize
			if ( nonce == NULL )
				return SAGDP_RET_NEED_NONCE;
			zepto_write_uint8( mem_h, SAGDP_P_STATUS_ERROR_MSG );
			// TODO: add other relevant data, if any
			sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
#endif
			INCREMENT_COUNTER( 73, "handler_sagdp_receive_hlp(), wait-remote, state/packet mismatch" );
			return SAGDP_RET_SYS_CORRUPTED;
		}

		if ( nonce == NULL )
			return SAGDP_RET_NEED_NONCE;

		ZEPTO_DEBUG_PRINTF_7( "handlerSAGDP_receivePID(): PID: %x%x%x%x%x%x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );

		// apply nonce
		bool is_prev = sa_uint48_compare( sagdp_data.first_last_sent_packet_id, sagdp_data.next_last_sent_packet_id ) != 0;
		if ( is_prev )
			sa_uint48_init_by( sagdp_data.prev_first_last_sent_packet_id, sagdp_data.first_last_sent_packet_id );
		else
			sa_uint48_init_by( sagdp_data.prev_first_last_sent_packet_id, nonce );
		sa_uint48_init_by( sagdp_data.first_last_sent_packet_id, nonce );
		sa_uint48_init_by( sagdp_data.next_last_sent_packet_id, nonce );

		// form a UP packet
		ZEPTO_DEBUG_ASSERT( ( packet_status & ( ~( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) ) == 0 ); // TODO: can we rely on sanity of the caller?
		parser_obj po1;
		zepto_parser_init_by_parser( &po1, &po );
		uint16_t body_size = zepto_parsing_remaining_bytes( &po );
		zepto_parse_skip_block( &po1, body_size );
		zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
		zepto_parser_encode_and_prepend_sa_uint48( mem_h, sagdp_data.last_received_packet_id );
		zepto_write_prepend_byte( mem_h, packet_status & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) );

		// save a copy
		zepto_copy_response_to_response_of_another_handle( mem_h, MEMORY_HANDLE_SAGDP_LSM );
		zepto_response_to_request( MEMORY_HANDLE_SAGDP_LSM );

		// request set timer
		if ( packet_status & SAGDP_P_STATUS_TERMINATING )
		{
			// just do nothing
			SAGDP_ASSERT_NO_SEQUENCE;
		}
		else
		{
			SAGDP_INIT_RESENT_SEQUENCE;
		}

		sagdp_data.state = packet_status == SAGDP_P_STATUS_TERMINATING ? SAGDP_STATE_IDLE : SAGDP_STATE_WAIT_REMOTE;
		INCREMENT_COUNTER( 74, "handler_sagdp_receive_hlp(), wait-remote, intermediate/terminating" );
		INCREMENT_COUNTER_IF( 75, "handler_sagdp_receive_hlp(), wait-remote, terminating", (packet_status >> 1) );
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
		sagdp_data.state = SAGDP_STATE_NOT_INITIALIZED;
		INCREMENT_COUNTER( 76, "handler_sagdp_receive_hlp(), invalid state" );
		return SAGDP_RET_SYS_CORRUPTED;
	}
}

