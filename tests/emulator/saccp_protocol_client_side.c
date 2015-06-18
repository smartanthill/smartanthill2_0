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


#include "saccp_protocol_client_side.h"
#include "../../firmware/src/common/sagdp_protocol.h" // for packet status in chain
#include "../../firmware/src/common/sa-uint48.h"
#include "../../firmware/src/common/saccp_protocol_constants.h"



#include "sa_test_control_prog.h"


#ifdef MASTER_ENABLE_ALT_TEST_MODE

void saccp_control_program_prepare_program( MEMORY_HANDLE mem_h, void* control_prog_state )
{
	// TODO: process
	zepto_write_block( mem_h, (const uint8_t*)"first message with a new design", 32 );
	zepto_parser_encode_and_prepend_uint16( mem_h, 32 ); // data size
	zepto_parser_encode_and_prepend_uint16( mem_h, 0 ); // body part ID
	zepto_write_prepend_byte( mem_h, ZEPTOVM_OP_EXEC );
}

uint8_t handler_sacpp_start_new_chain( MEMORY_HANDLE mem_h, void* control_prog_state )
{
	default_test_control_program_start_new( control_prog_state, mem_h );
	return SACCP_RET_PASS_LOWER;
}

uint8_t handler_sacpp_continue_chain( MEMORY_HANDLE mem_h, void* control_prog_state )
{
	default_test_control_program_accept_reply_continue( control_prog_state, mem_h );
	return SACCP_RET_PASS_LOWER;
}

#else // MASTER_ENABLE_ALT_TEST_MODE

uint8_t handler_saccp_prepare_to_send( MEMORY_HANDLE mem_h )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	uint8_t first_byte = zepto_parse_uint8( &po );
	uint16_t sz = zepto_parsing_remaining_bytes( &po );
	zepto_parser_init_by_parser( &po1, &po );
	zepto_parse_skip_block( &po, sz );
	zepto_convert_part_of_request_to_response( mem_h, &po1, &po );
	uint8_t hdr = SACCP_NEW_PROGRAM; //TODO: we may want to add extra headers
	zepto_write_prepend_byte( mem_h, hdr );
//	zepto_write_prepend_byte( mem_h, SAGDP_P_STATUS_FIRST );
	zepto_write_prepend_byte( mem_h, first_byte );
	return SACCP_RET_PASS_LOWER;
}

#endif // MASTER_ENABLE_ALT_TEST_MODE

#ifdef MASTER_ENABLE_ALT_TEST_MODE
uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id, void* control_prog_state )
#else // MASTER_ENABLE_ALT_TEST_MODE
uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h )
#endif // MASTER_ENABLE_ALT_TEST_MODE
{
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	uint8_t first_byte = zepto_parse_uint8( &po );
	uint16_t packet_head = zepto_parse_encoded_uint16( &po );
	uint8_t packet_type = packet_head & 0x7; // TODO: use bit field processing instead

	switch ( packet_type )
	{
		case SACCP_PAIRING_RESPONSE:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_PROGRAMMING_RESPONSE:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_REPLY_OK:
		{
/*			if ( packet_head_byte & 0xF0 ) // TODO: use bit field processing instead
			{
				form_error_packet( mem_h, SACCP_ERROR_INVALID_FORMAT, first_byte & SAGDP_P_STATUS_MASK, chain_id ); // TODO: use bit field processing instead
				return SACCP_RET_OK; //+++ TODO: should it be FAILED?
			}*/
			uint8_t is_truncated = packet_head & 0x8; // TODO: use bit field processing instead
			uint16_t data_full_sz = packet_head >> 4;
			ZEPTO_DEBUG_ASSERT( data_full_sz == zepto_parsing_remaining_bytes( &po ) ); // TODO: can it be not so?
			if ( is_truncated != 0 ) // TODO: use bit field processing instead
			{
				ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			}
			if ( data_full_sz ) // TODO: is it legitimate to have here zero
			{
				bool more_frames = true;
				// read frames one by one until the end of the packet
				do
				{
					uint16_t hh = zepto_parse_encoded_uint16( &po );
					// TODO: use bit field processing instead in the code below where applicable
					uint16_t frame_sz = hh >> 2;
					// TODO: truncated flag
					uint8_t is_last = hh & 1;
					if ( is_last == 1 )
					{
						parser_obj po1;
						zepto_parser_init_by_parser( &po1, &po );
						zepto_parse_skip_block( &po, frame_sz );
#ifdef MASTER_ENABLE_ALT_TEST_MODE
						// we are in the scope of central unit; process in-place
						default_test_control_program_accept_reply( control_prog_state, first_byte & SAGDP_P_STATUS_MASK, &po1, frame_sz );
#else // MASTER_ENABLE_ALT_TEST_MODE
						// we are in the scope of commstack; prepare for sending to central unit
						ZEPTO_DEBUG_ASSERT( zepto_parsing_remaining_bytes( &po ) == 0 ); // multi-frame responses are not yet implemented
						zepto_convert_part_of_request_to_response( mem_h, &po1, &po );
						return SACCP_RET_PASS_TO_CENTRAL_UNIT;
#endif // MASTER_ENABLE_ALT_TEST_MODE
					}
					else
					{
						ZEPTO_DEBUG_ASSERT( NULL == "error: not implemented" );
					}
				}
				while ( zepto_parsing_remaining_bytes( &po ) );
			}
#ifdef MASTER_ENABLE_ALT_TEST_MODE
			return (first_byte & SAGDP_P_STATUS_MASK) == SAGDP_P_STATUS_TERMINATING ? SACCP_RET_CHAIN_DONE : SACCP_RET_CHAIN_CONTINUED;
#else // MASTER_ENABLE_ALT_TEST_MODE
			return SACCP_RET_FAILED;
#endif // MASTER_ENABLE_ALT_TEST_MODE
			break;
		}
		case SACCP_REPLY_EXCEPTION:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_REPLY_ERROR:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		default:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: unexpected value of packet type\n" );
		}
	}

}

