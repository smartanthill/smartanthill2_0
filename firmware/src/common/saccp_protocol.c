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


#include "saccp_protocol.h"
#include "sagdp_protocol.h" // for packet status in chain
#include "sa-uint48.h"
#include "saccp_protocol_constants.h"


//#include "../plugins/smart-echo/smart-echo.h"
#include "../sa_bodypart_list.h"

//SmartEchoPluginConfig pl_conf;
//SmartEchoPluginState pl_state;

void zepto_vm_init()
{
//	smart_echo_plugin_handler_init( (void*)(&pl_conf), (void*)(&pl_state) );
	smart_echo_plugin_handler_init( (void*)(bodyparts[0].ph_config), (void*)(bodyparts[0].ph_state) );
}



void handler_zepto_test_plugin( MEMORY_HANDLE mem_h )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );
	zepto_parser_init( &po1, mem_h );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );

	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
}

void handler_zepto_vm( MEMORY_HANDLE mem_h, uint8_t first_byte )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	uint8_t op_code;
	bool explicit_exit_called = false;
	uint8_t reply_packet_in_chain_flags = SAGDP_P_STATUS_TERMINATING;
	do
	{
		if ( zepto_parsing_remaining_bytes( &po ) == 0 )
			break;

		op_code = zepto_parse_uint8( &po );
		switch( op_code )
		{
			case ZEPTOVM_OP_EXEC:
			{
//				int16_t body_part = zepto_parse_encoded_int16( &po );
				// TODO: code below is HIGHLY temporary stub and should be replaced by the commented line above (with proper implementation of the respective function ASAP
				// (for the sake of quick progress of mainstream development currently we assume that the value of body_part is within single +/- decimal digit)
				uint16_t body_part = zepto_parse_encoded_uint16( &po );
				ZEPTO_DEBUG_ASSERT( body_part < 128 );
				body_part -= 64;

				uint16_t data_sz = zepto_parse_encoded_uint16( &po );

				//+++ TODO: rethink memory management
				zepto_parser_init_by_parser( &po1, &po );
//				zepto_parse_skip_block( &po, zepto_parsing_remaining_bytes( &po ) );
				zepto_parse_skip_block( &po, data_sz );
				zepto_copy_part_of_request_to_response_of_another_handle( mem_h, &po1, &po, MEMORY_HANDLE_DEFAULT_PLUGIN );
				zepto_response_to_request( MEMORY_HANDLE_DEFAULT_PLUGIN );

//				handler_zepto_test_plugin( MEMORY_HANDLE_DEFAULT_PLUGIN );
				parser_obj po3;
				zepto_parser_init( &po3, MEMORY_HANDLE_DEFAULT_PLUGIN );
				smart_echo_plugin_handler( (void*)(bodyparts[0].ph_config), (void*)(bodyparts[0].ph_state), &po3, MEMORY_HANDLE_DEFAULT_PLUGIN/*, WaitingFor* waiting_for*/, first_byte );
				// now we have raw data from plugin; form a frame
				// TODO: here is a place to form optional headers, if any
				uint16_t ret_data_sz = zepto_writer_get_response_size( MEMORY_HANDLE_DEFAULT_PLUGIN );
				uint16_t prefix = (uint16_t)1 | ( ret_data_sz << 2 ); // TODO: if data were truncated, add a respective bit; TODO: usi bit field processing instead
				zepto_parser_encode_and_prepend_uint16( MEMORY_HANDLE_DEFAULT_PLUGIN, prefix );

				zepto_append_response_to_response_of_another_handle( MEMORY_HANDLE_DEFAULT_PLUGIN, mem_h );
				zepto_response_to_request( MEMORY_HANDLE_DEFAULT_PLUGIN );
				zepto_response_to_request( MEMORY_HANDLE_DEFAULT_PLUGIN );
				break;
			}
			case ZEPTOVM_OP_EXIT:
			{
				explicit_exit_called = true;
				uint16_t flags = zepto_parse_uint8( &po );
				reply_packet_in_chain_flags = flags & 3;
				break;
			}
			case ZEPTOVM_OP_DEVICECAPS:
			case ZEPTOVM_OP_PUSHREPLY:
			case ZEPTOVM_OP_SLEEP:
			case ZEPTOVM_OP_TRANSMITTER:
			case ZEPTOVM_OP_MCUSLEEP:
			case ZEPTOVM_OP_POPREPLIES:
			case ZEPTOVM_OP_APPENDTOREPLY:

			case ZEPTOVM_OP_JMP:
			case ZEPTOVM_OP_JMPIFREPLYFIELD_LT:
			case ZEPTOVM_OP_JMPIFREPLYFIELD_GT:
			case ZEPTOVM_OP_JMPIFREPLYFIELD_EQ:
			case ZEPTOVM_OP_JMPIFREPLYFIELD_NE:
			case ZEPTOVM_OP_MOVEREPLYTOFRONT:
			case ZEPTOVM_OP_PUSHEXPR_CONSTANT:
			case ZEPTOVM_OP_PUSHEXPR_REPLYFIELD:
			case ZEPTOVM_OP_EXPRUNOP:
			case ZEPTOVM_OP_EXPRUNOP_EX:
			case ZEPTOVM_OP_EXPRUNOP_EX2:
			case ZEPTOVM_OP_EXPRBINOP:
			case ZEPTOVM_OP_EXPRBINOP_EX:
			case ZEPTOVM_OP_EXPRBINOP_EX2:
			case ZEPTOVM_OP_JMPIFEXPR_LT:
			case ZEPTOVM_OP_JMPIFEXPR_GT:
			case ZEPTOVM_OP_JMPIFEXPR_EQ:
			case ZEPTOVM_OP_JMPIFEXPR_NE:
			case ZEPTOVM_OP_JMPIFEXPR_EX_LT:
			case ZEPTOVM_OP_JMPIFEXPR_EX_GT:
			case ZEPTOVM_OP_JMPIFEXPR_EX_EQ:
			case ZEPTOVM_OP_JMPIFEXPR_EX_NE:
			case ZEPTOVM_OP_CALL:
			case ZEPTOVM_OP_RET:
			case ZEPTOVM_OP_SWITCH:
			case ZEPTOVM_OP_SWITCH_EX:
			case ZEPTOVM_OP_INCANDJMPIF:
			case ZEPTOVM_OP_DECANDJMPIF:
			case ZEPTOVM_OP_PARALLEL:
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
	while ( !explicit_exit_called );

/*	if ( explicit_exit_called )
	{
		ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
	}
	else*/
	{
		uint16_t ret_data_full_sz = zepto_writer_get_response_size( mem_h );
		uint16_t reply_hdr;
		// TODO: it's a place to set TRUNCATED flag for the whole reply, if necessary
		// TODO: use bit field processing instead
		reply_hdr = SACCP_REPLY_OK | (ret_data_full_sz << 4);
		zepto_parser_encode_and_prepend_uint16( mem_h, reply_hdr );
		zepto_write_prepend_byte( mem_h, reply_packet_in_chain_flags );
	}
}

INLINE
void form_error_packet( MEMORY_HANDLE mem_h, uint8_t error_code, uint8_t incoming_packet_status, sasp_nonce_type chain_id )
{
	ZEPTO_DEBUG_ASSERT( ( error_code & 0xF8 ) == 0 );
	if ( incoming_packet_status != SAGDP_P_STATUS_TERMINATING )
	{
		uint16_t body = error_code;
		body <<= 4;
		body |= SACCP_REPLY_ERROR;
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_TERMINATING );
		zepto_parser_encode_and_append_uint16( mem_h, body );
	}
	else
	{
		uint16_t body = error_code;
		body <<= 1;
		body |= 1; // extra data present
		body <<= 3;
		body |= SACCP_REPLY_ERROR;
		zepto_write_uint8( mem_h, SAGDP_P_STATUS_FIRST );
		zepto_parser_encode_and_append_uint16( mem_h, body );
		zepto_parser_encode_and_append_sa_uint48( mem_h, chain_id );
	}
}

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id )
{
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	uint8_t first_byte = zepto_parse_uint8( &po );
	uint8_t packet_head_byte = zepto_parse_uint8( &po );
	uint8_t packet_type = packet_head_byte & 0x7; // TODO: use bit field processing instead

	switch ( packet_type )
	{
		case SACCP_PAIRING:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_PROGRAMMING:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_NEW_PROGRAM:
		{
			if ( packet_head_byte & 0xF0 ) // TODO: use bit field processing instead
			{
				form_error_packet( mem_h, SACCP_ERROR_INVALID_FORMAT, first_byte & SAGDP_P_STATUS_MASK, chain_id ); // TODO: use bit field processing instead
				return SACCP_RET_PASS_LOWER; //+++ TODO: should it be FAILED?
			}
			uint8_t are_headers = packet_head_byte & 0x8; // TODO: use bit field processing instead
			if ( are_headers != 0 ) // TODO: use bit field processing instead
			{
				bool more_headers = true;
				// read headers one by one until terminating is found
				do
				{
					uint16_t hh = zepto_parse_encoded_uint16( &po );
					// TODO: use bit field processing instead in the code below where applicable
					uint8_t h_type = hh & 0x7;
					uint16_t sz = hh >> 3;
					switch ( h_type )
					{
						case END_OF_HEADERS:
						{
							more_headers = false;
							if ( sz ) // cannot happen in a valid packet
							{
								form_error_packet( mem_h, SACCP_ERROR_INVALID_FORMAT, first_byte & SAGDP_P_STATUS_MASK, chain_id ); // TODO: use bit field processing instead
								return SACCP_RET_PASS_LOWER; //+++ TODO: should it be FAILED?
							}
							break;
						}
						case ENABLE_ZEPTOERR:
						{
							ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
							// TODO: read data according to 'sz'; process it
							break;
						}
						default:
						{
							ZEPTO_DEBUG_ASSERT( NULL == "Error: unexpected value of extra header type\n" );
						}
					}
				}
				while ( more_headers );
			}

			// now a packet body remains; it will be forwarded to Execution-Layer-Program
			parser_obj po1;
			zepto_parser_init_by_parser( &po1, &po );
			zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
			zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
			zepto_response_to_request( mem_h );

			handler_zepto_vm( mem_h, first_byte ); // TODO: it can be implemented as an additional layer
			return SACCP_RET_PASS_LOWER;
			break;
		}
		case SACCP_REPEAT_OLD_PROGRAM:
		{
			ZEPTO_DEBUG_ASSERT( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_REUSE_OLD_PROGRAM:
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
/*
uint8_t handler_sacpp_reply( MEMORY_HANDLE mem_h )
{
}
*/
