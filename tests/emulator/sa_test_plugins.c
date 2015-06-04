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


#include "sa_test_plugins.h"
#include "sagdp_protocol.h" // for packet flags

#include "test-generator.h"
#include <stdio.h> // for sprintf() in fake implementation

uint8_t default_test_plugin_handler_init( const void* plugin_config, void* plugin_state )
{
	//perform sensor initialization if necessary
	DefaultTestingPluginState* ps = (DefaultTestingPluginState*)plugin_state;
	ps->state = 0;
	ps->currChainIdBase[0] = 0;
	ps->currChainIdBase[1] = MASTER_SLAVE_BIT << 15;
	return PLUGIN_OK;
}


uint8_t default_test_plugin_handler_continue( const void* plugin_config, void* plugin_state, parser_obj* command, MEMORY_HANDLE reply )
{
	const DefaultTestingPluginConfig* pc = (DefaultTestingPluginConfig*) plugin_config;
	DefaultTestingPluginState* ps = (DefaultTestingPluginState*)plugin_state;

	zepto_response_to_request( reply );

	zepto_write_uint8( reply, ps->first_byte );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[0] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[1] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_ini_size );
	zepto_parser_encode_and_append_uint16( reply, ps->reply_to_id );
	zepto_parser_encode_and_append_uint16( reply, ps->self_id );

	char tail[256];
	uint16_t varln = 6 - ps->self_id % 7; // 0:6
	uint8_t i;
	for ( i=0;i<varln; i++ )
		tail[ i] = '-';
	tail[ varln ] = '>';
	tail[ varln + 1 ] = 0;
	zepto_write_block( reply, (uint8_t*)tail, varln + 1 );
	uint16_t msg_size = 11+varln+1;

	// print outgoing packet
	PRINTF( "Yocto: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1], ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );

	assert( msg_size >= 7 && msg_size <= 22 );

	INCREMENT_COUNTER( 1, "slave_process(), packet sent" );

	// return status
//	chainContinued = true;
	return PLUGIN_PASS_LOWER;
}


uint8_t default_test_plugin_handler( const void* plugin_config, void* plugin_state, parser_obj* command, MEMORY_HANDLE reply/*, WaitingFor* waiting_for*/, uint8_t first_byte ) 
{
	const DefaultTestingPluginConfig* pc = (DefaultTestingPluginConfig*) plugin_config;
	DefaultTestingPluginState* ps = (DefaultTestingPluginState*)plugin_state;

	if ( ps->state == 0 )
	{
		uint16_t msg_size = zepto_parsing_remaining_bytes( command ); // all these bytes + (potentially) {padding_size + padding} will be written
//		ps->first_byte = zepto_parse_uint8( command );
		ps->first_byte = first_byte;
		if ( ( ps->first_byte & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_ERROR_MSG )
		{
			PRINTF( "slave_process(): ERROR MESSAGE RECEIVED IN YOCTO\n" );
			assert(0);
		}

		ps->chain_id[0] = zepto_parse_encoded_uint16( command );
		ps->chain_id[1] = zepto_parse_encoded_uint16( command );
		ps->chain_ini_size = zepto_parse_encoded_uint16( command );
		ps->reply_to_id = zepto_parse_encoded_uint16( command );
		ps->self_id = zepto_parse_encoded_uint16( command );
		char tail[256];
		uint16_t tail_sz = zepto_parsing_remaining_bytes( command );
		zepto_parse_read_block( command, (uint8_t*)tail, tail_sz );
		tail[ tail_sz ] = 0;

		// print packet
		PRINTF( "Yocto: Packet received: [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1], ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );

		// test and analyze

		// size
		if ( !( msg_size >= 7 && msg_size <= 22 ) )
			printf( "ZEPTO: BAD PACKET RECEIVED\n", msg_size );
		assert( msg_size >= 7 && msg_size <= 22 );

		// flags
		assert( ( ps->first_byte & 4 ) == 0 );
		ps->first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
		if ( ps->first_byte == SAGDP_P_STATUS_FIRST )
		{
			assert( 0 == ps->reply_to_id );
			assert( ps->chain_id[0] != ps->currChainID[0] || ps->chain_id[1] != ps->currChainID[1] );
			ps->currChainID[0] = ps->chain_id[0];
			ps->currChainID[1] = ps->chain_id[1];
		}
		else
		{
			assert( ps->last_sent_id == ps->reply_to_id );
			assert( ps->chain_id[0] == ps->currChainID[0] && ps->chain_id[1] == ps->currChainID[1] );
		}

		if ( ps->first_byte == SAGDP_P_STATUS_TERMINATING )
		{
	//		chainContinued = false;
			ps->currChainIdBase[0] ++;
			return PLUGIN_OK;
		}

		// fake implementation: should this packet be terminal?
		if ( ps->chain_ini_size == ps->self_id + 1 )
			ps->first_byte = SAGDP_P_STATUS_TERMINATING;
		else
			ps->first_byte = SAGDP_P_STATUS_INTERMEDIATE;

		// prepare outgoing packet
		ps->reply_to_id = ps->self_id;
		(ps->self_id)++;
		ps->last_sent_id = ps->self_id;

		// scenario decision
//		if ( 0 )
		{
			// just go through
//			*wait_to_process_time = 0;
			return default_test_plugin_handler_continue( plugin_config, plugin_state, command, reply );
		}
/*		else
		{
			// request to wait
			*wait_to_process_time = tester_get_rand_val() % 5;
			return PLUGIN_WAIT_TO_CONTINUE;
		}*/
	}
	else
	{
		assert( NULL == "unknown state\n" );
	}

	return 0;
}
