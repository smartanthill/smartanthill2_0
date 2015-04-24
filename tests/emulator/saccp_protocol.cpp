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


#define SACCP_PAIRING 0x0            /*Master: sends; Slave: receives*/
#define SACCP_PROGRAMMING 0x1        /*Master: sends; Slave: receives*/
#define SACCP_NEW_PROGRAM 0x2        /*Master: sends; Slave: receives*/
#define SACCP_REPEAT_OLD_PROGRAM 0x3 /*Master: sends; Slave: receives*/
#define SACCP_REUSE_OLD_PROGRAM 0x4  /*Master: sends; Slave: receives*/

#define SACCP_PAIRING_RESPONSE 0x7     /*Master: receives; Slave: sends*/
#define SACCP_PROGRAMMING_RESPONSE 0x6 /*Master: receives; Slave: sends*/
#define SACCP_REPLY_OK 0x0             /*Master: receives; Slave: sends*/
#define SACCP_REPLY_EXCEPTION 0x1      /*Master: receives; Slave: sends*/
#define SACCP_REPLY_ERROR 0x2          /*Master: receives; Slave: sends*/


// extra headers types
#define END_OF_HEADERS 0
#define ENABLE_ZEPTOERR 1


// error codes
#define SACCP_ERROR_INVALID_FORMAT 1
#define SACCP_ERROR_OLD_PROGRAM_CHECKSUM_DOESNT_MATCH 2


// op codes

#define ZEPTOVM_OP_DEVICECAPS 0x1
#define ZEPTOVM_OP_EXEC 0x2
#define ZEPTOVM_OP_PUSHREPLY 0x3
#define ZEPTOVM_OP_SLEEP 0x4
#define ZEPTOVM_OP_TRANSMITTER 0x5
#define ZEPTOVM_OP_MCUSLEEP 0x6
#define ZEPTOVM_OP_POPREPLIES 0x7  /* limited support in Zepto VM-One, full support from Zepto VM-Tiny */
#define ZEPTOVM_OP_EXIT 0x8
#define ZEPTOVM_OP_APPENDTOREPLY 0x9 /* limited support in Zepto VM-One, full support from Zepto VM-Tiny */
/* starting from the next opcode, instructions are not supported by Zepto VM-One */
#define ZEPTOVM_OP_JMP 0xA
#define ZEPTOVM_OP_JMPIFREPLYFIELD_LT 0xB
#define ZEPTOVM_OP_JMPIFREPLYFIELD_GT 0xC
#define ZEPTOVM_OP_JMPIFREPLYFIELD_EQ 0xD
#define ZEPTOVM_OP_JMPIFREPLYFIELD_NE 0xE
#define ZEPTOVM_OP_MOVEREPLYTOFRONT 0xF
/* starting from the next opcode, instructions are not supported by Zepto VM-Tiny and below */
#define ZEPTOVM_OP_PUSHEXPR_CONSTANT 0x10
#define ZEPTOVM_OP_PUSHEXPR_REPLYFIELD 0x11
#define ZEPTOVM_OP_EXPRUNOP 0x12
#define ZEPTOVM_OP_EXPRUNOP_EX 0x13
#define ZEPTOVM_OP_EXPRUNOP_EX2 0x14
#define ZEPTOVM_OP_EXPRBINOP 0x15
#define ZEPTOVM_OP_EXPRBINOP_EX 0x16
#define ZEPTOVM_OP_EXPRBINOP_EX2 0x17
#define ZEPTOVM_OP_JMPIFEXPR_LT 0x18
#define ZEPTOVM_OP_JMPIFEXPR_GT 0x19
#define ZEPTOVM_OP_JMPIFEXPR_EQ 0x1A
#define ZEPTOVM_OP_JMPIFEXPR_NE 0x1B
#define ZEPTOVM_OP_JMPIFEXPR_EX_LT 0x1C
#define ZEPTOVM_OP_JMPIFEXPR_EX_GT 0x1D
#define ZEPTOVM_OP_JMPIFEXPR_EX_EQ 0x1E
#define ZEPTOVM_OP_JMPIFEXPR_EX_NE 0x1F
#define ZEPTOVM_OP_CALL 0x20
#define ZEPTOVM_OP_RET 0x21
#define ZEPTOVM_OP_SWITCH 0x22
#define ZEPTOVM_OP_SWITCH_EX 0x23
#define ZEPTOVM_OP_INCANDJMPIF 0x24
#define ZEPTOVM_OP_DECANDJMPIF 0x25
/* starting from the next opcode, instructions are not supported by Zepto VM-Small and below */
#define ZEPTOVM_OP_PARALLEL 0x26





void handler_zepto_test_plugin( MEMORY_HANDLE mem_h )
{
}

void handler_zepto_vm( MEMORY_HANDLE mem_h )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	uint8_t op_code;
	bool explicit_exit_called = false;
	bool commands_remain = true;
	do
	{
		op_code = zepto_parse_uint8( &po );
		switch( op_code )
		{
			case ZEPTOVM_OP_EXEC:
			{
				break;
	//			int16_t body_part = zepto_parse_encoded_int16( &po );
				// TODO: code below is HIGHLY temporary stub and should be replaced by the commented line above (with proper implementation of the respective function ASAP
				// (for the sake of quick progress of mainstream development currently we assume that the value of body_part is within single +/- decimal digit)
				uint16_t body_part = zepto_parse_uint8( &po );
				assert( body_part < 128 );
				body_part -= 64;

				uint16_t data_sz = zepto_parse_encoded_uint16( &po );

				//+++ TODO: rethink memory management
				zepto_parser_init( &po1, &po );
				zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
				zepto_copy_part_of_request_to_response_of_another_handle( mem_h, &po, &po1, MEMORY_HANDLE_DEFAULT_PLUGIN );
				zepto_response_to_request( MEMORY_HANDLE_DEFAULT_PLUGIN );

				handler_zepto_test_plugin( MEMORY_HANDLE_DEFAULT_PLUGIN );

				zepto_append_part_of_request_to_response_of_another_handle( MEMORY_HANDLE_DEFAULT_PLUGIN, mem_h );
			}
			case ZEPTOVM_OP_DEVICECAPS:
			case ZEPTOVM_OP_PUSHREPLY:
			case ZEPTOVM_OP_SLEEP:
			case ZEPTOVM_OP_TRANSMITTER:
			case ZEPTOVM_OP_MCUSLEEP:
			case ZEPTOVM_OP_POPREPLIES:
			case ZEPTOVM_OP_EXIT:
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
				assert( NULL == "Error: not implemented\n" );
				break;
			}
			default:
			{
				assert( NULL == "Error: unexpected value of packet type\n" );
			}
		}
	}
	while ( commands_remain );
}

inline
void form_error_packet( MEMORY_HANDLE mem_h, uint8_t error_code, uint8_t incoming_packet_status, sasp_nonce_type chain_id )
{
	assert( ( error_code & 0xF8 ) == 0 );
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
		case SACCP_PAIRING_RESPONSE:
		{
			assert( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_PROGRAMMING:
		{
			assert( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_NEW_PROGRAM:
		{
			if ( packet_head_byte & 0xF0 ) // TODO: use bit field processing instead
			{
				form_error_packet( mem_h, SACCP_ERROR_INVALID_FORMAT, first_byte & SAGDP_P_STATUS_MASK, chain_id ); // TODO: use bit field processing instead
				return SACCP_RET_OK; //+++ TODO: should it be FAILED?
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
					uint8_t h_type = hh && 0x7;
					uint16_t sz = hh >> 3;
					switch ( h_type )
					{
						case END_OF_HEADERS:
						{
							more_headers = false;
							if ( sz ) // cannot happen in a valid packet
							{
								form_error_packet( mem_h, SACCP_ERROR_INVALID_FORMAT, first_byte & SAGDP_P_STATUS_MASK, chain_id ); // TODO: use bit field processing instead
								return SACCP_RET_OK; //+++ TODO: should it be FAILED?
							}
							break;
						}
						case ENABLE_ZEPTOERR:
						{
							assert( NULL == "Error: not implemented\n" );
							// TODO: read data according to 'sz'; process it
							break;
						}
						default:
						{
							assert( NULL == "Error: unexpected value of extra header type\n" );
						}
					}
				}
				while ( more_headers );

				// now a packet body remains; it will be forwarded to Execution-Layer-Program
				parser_obj po1;
				zepto_parser_init( &po1, &po );
				zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
				zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
				zepto_response_to_request( mem_h );

				handler_zepto_vm( mem_h ); // TODO: it can be implemented as an additional layer
			}
			break;
		}
		case SACCP_REPEAT_OLD_PROGRAM:
		{
			assert( NULL == "Error: not implemented\n" );
			break;
		}
		case SACCP_REUSE_OLD_PROGRAM:
		{
			assert( NULL == "Error: not implemented\n" );
			break;
		}
		default:
		{
			assert( NULL == "Error: unexpected value of packet type\n" );
		}
	}
}
/*
uint8_t handler_sacpp_reply( MEMORY_HANDLE mem_h )
{
}
*/
