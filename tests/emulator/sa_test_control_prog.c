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
#ifdef MASTER_ENABLE_ALT_TEST_MODE

#include "sa_test_control_prog.h"
#include "../../firmware/src/common/sagdp_protocol.h" // for packet flags
#include "../../firmware/src/common/saccp_protocol_constants.h"

#include "test_generator.h"
#include <stdio.h> // for sprintf() in fake implementation

#define CHAIN_MAX_SIZE 9
//#define MANUAL_TEST_DATA_ENTERING

uint8_t default_test_control_program_init( void* control_prog_state )
{
	//perform sensor initialization if necessary
	DefaultTestingControlProgramState* ps = (DefaultTestingControlProgramState*)control_prog_state;
	ps->state = 0;
	ps->currChainIdBase[0] = 0;
	ps->currChainIdBase[1] = MASTER_SLAVE_BIT << 15;
	return CONTROL_PROG_OK;
}


uint8_t default_test_control_program_start_new( void* control_prog_state, MEMORY_HANDLE reply )
{
	// Forms a first packet in the chain
	// for structure of the packet see comments to yocto_process()
	// Initial number of packets in the chain is currently entered manually by a tester
	PRINT_COUNTERS();

	DefaultTestingControlProgramState* ps = (DefaultTestingControlProgramState*)control_prog_state;

	ps->first_byte = SAGDP_P_STATUS_FIRST;

#ifdef MANUAL_TEST_DATA_ENTERING

	uint8_t chain_ini_size = 0;
	while ( chain_ini_size == 0 || chain_ini_size == '\n' )
		chain_ini_size = getchar();
	if ( chain_ini_size == 'x' )
		return CONTROL_PROG_FAILED;
	chain_ini_size -= '0';
	chainContinued = true;

#else // MANUAL_TEST_DATA_ENTERING

	ps->chain_ini_size = tester_get_rand_val() % ( CHAIN_MAX_SIZE - 2 ) + 2;

#endif // MANUAL_TEST_DATA_ENTERING

	ps->currChainID[0] = ++( ps->currChainIdBase[0] );
	ps->currChainID[1] = ps->currChainIdBase[1];
	ps->chain_id[0] = ps->currChainID[0];
	ps->chain_id[1] = ps->currChainID[1];
	ps->reply_to_id = 0;
	ps->self_id = 1;
	ps->last_sent_id = ps->self_id;

	// prepare outgoing packet
//	zepto_write_uint8( reply, ps->first_byte );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[0] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[1] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_ini_size );
	zepto_parser_encode_and_append_uint16( reply, ps->reply_to_id );
	zepto_parser_encode_and_append_uint16( reply, ps->self_id );
	uint8_t varln = 6 - ps->self_id % 7; // 0:6
	zepto_write_uint8( reply, varln + 1 );

	char tail[256];
	uint8_t i;
	for ( i=0;i<varln; i++ )
		tail[ i] = '-';
	tail[ varln ] = '>';
	tail[ varln + 1 ] = 0;
	zepto_write_block( reply, (uint8_t*)tail, varln + 1 );

	// now the core is ready, and we are about to finalize the "program" 
//	uint16_t msg_size = 11+varln+1;
	uint16_t reply_sz = zepto_writer_get_response_size( reply );
	zepto_parser_encode_and_prepend_uint16( reply, reply_sz ); // data size
	zepto_parser_encode_and_prepend_uint16( reply, 0 ); // body part ID (default so far); TODO: determination
	zepto_write_prepend_byte( reply, ZEPTOVM_OP_EXEC );

	// print outgoing packet
//	PRINTF( "Zepto-Master: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", reply_sz, ps->first_byte, ps->chain_id[0], ps->chain_id[1], ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );
	ZEPTO_DEBUG_PRINTF_5( "Zepto-Master: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x]",  reply_sz, ps->first_byte, ps->chain_id[0], ps->chain_id[1] );
	ZEPTO_DEBUG_PRINTF_5( "[0x%04x][0x%04x][0x%04x]%s\n", ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );
	ZEPTO_DEBUG_ASSERT( reply_sz >= 7 && reply_sz <= 22 );

	if ( ps->chain_ini_size > 2 ) // a "complex" chain
	{
		// need to add explicit exit command to specify packet status-in-chain of the reply
		zepto_write_uint8( reply, ZEPTOVM_OP_EXIT );
		zepto_write_uint8( reply, (uint8_t)SAGDP_P_STATUS_INTERMEDIATE ); // TODO: if padding is required, add necessary data here
//		reply_sz = zepto_writer_get_response_size( reply );
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( ps->chain_ini_size == 2 );
	}
/*	uint8_t hdr = SACCP_NEW_PROGRAM; //TODO: we may want to add extra headers
	zepto_write_prepend_byte( reply, hdr );*/
	zepto_write_prepend_byte( reply, SAGDP_P_STATUS_FIRST );

	INCREMENT_COUNTER( 3, "master_start(), chain started" );

	// return status
	return CONTROL_PROG_PASS_LOWER;
}


uint8_t default_test_control_program_accept_reply_continue( void* control_prog_state, MEMORY_HANDLE reply )
{
	DefaultTestingControlProgramState* ps = (DefaultTestingControlProgramState*)control_prog_state;

	zepto_response_to_request( reply );

//	zepto_write_uint8( reply, ps->first_byte );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[0] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_id[1] );
	zepto_parser_encode_and_append_uint16( reply, ps->chain_ini_size );
	zepto_parser_encode_and_append_uint16( reply, ps->reply_to_id );
	zepto_parser_encode_and_append_uint16( reply, ps->self_id );
	uint8_t varln = 6 - ps->self_id % 7; // 0:6
	zepto_write_uint8( reply, varln + 1 );

	char tail[256];
	uint8_t i;
	for ( i=0;i<varln; i++ )
		tail[ i] = '-';
	tail[ varln ] = '>';
	tail[ varln + 1 ] = 0;
	zepto_write_block( reply, (uint8_t*)tail, varln + 1 );
	uint16_t msg_size = 11+varln+1;

	uint16_t reply_sz = zepto_writer_get_response_size( reply );
	zepto_parser_encode_and_prepend_uint16( reply, reply_sz ); // data size
	zepto_parser_encode_and_prepend_uint16( reply, 0 ); // body part ID (default so far); TODO: determination
	zepto_write_prepend_byte( reply, ZEPTOVM_OP_EXEC );

	// print outgoing packet
//	PRINTF( "Yocto: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1], ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );
	ZEPTO_DEBUG_PRINTF_5( "Zepto-Master: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x]",  msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1] );
	ZEPTO_DEBUG_PRINTF_5( "[0x%04x][0x%04x][0x%04x]%s\n", ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );
	ZEPTO_DEBUG_ASSERT( msg_size >= 7 && msg_size <= 22 );

	INCREMENT_COUNTER( 7, "master_continue(), packet sent" );
	INCREMENT_COUNTER_IF( 8, "master_continue(), last packet sent", (ps->first_byte >> 1) );

	if ( ps->chain_ini_size > ps->self_id + 1 ) // a "complex" chain
	{
		// need to add explicit exit command to specify packet status-in-chain of the reply
		zepto_write_uint8( reply, ZEPTOVM_OP_EXIT );
		zepto_write_uint8( reply, (uint8_t)SAGDP_P_STATUS_INTERMEDIATE ); // TODO: if padding is required, add necessary data here
		reply_sz = zepto_writer_get_response_size( reply );
	}
	else
	{
//		ZEPTO_DEBUG_ASSERT( ps->chain_ini_size == 2 );
		// need to add explicit exit command to specify packet status-in-chain of the reply
		zepto_write_uint8( reply, ZEPTOVM_OP_EXIT );
		zepto_write_uint8( reply, (uint8_t)SAGDP_P_STATUS_TERMINATING ); // TODO: if padding is required, add necessary data here
		reply_sz = zepto_writer_get_response_size( reply );
	}
/*	uint8_t hdr = SACCP_NEW_PROGRAM; //TODO: we may want to add extra headers
	zepto_write_prepend_byte( reply, hdr );*/
	zepto_write_prepend_byte( reply, SAGDP_P_STATUS_INTERMEDIATE );


	// return status
	return ps->first_byte == SAGDP_P_STATUS_TERMINATING ? CONTROL_PROG_PASS_LOWER_THEN_IDLE : CONTROL_PROG_PASS_LOWER; 
}

uint8_t _default_test_control_program_accept_reply( void* control_prog_state, uint8_t packet_status, parser_obj* received, uint16_t msg_size )
{
	// by now master_continue() does the same as yocto_process
//	*wait_to_process_time = 0;

	DefaultTestingControlProgramState* ps = (DefaultTestingControlProgramState*)control_prog_state;

	INCREMENT_COUNTER( 4, "master_continue(), packet received" );

//	uint16_t msg_size = zepto_parsing_remaining_bytes( received ); // all these bytes + (potentially) {padding_size + padding} will be written
	ps->first_byte = packet_status;
//	ps->first_byte = zepto_parse_uint8( received );
//	ps->first_byte = zepto_parse_uint8( received );
	if ( ( ps->first_byte & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_ERROR_MSG )
	{
		ZEPTO_DEBUG_PRINTF_1( "_default_test_control_program_accept_reply(): ERROR MESSAGE RECEIVED IN ZEPTO\n" );
		(ps->currChainIdBase[0]) ++;
		INCREMENT_COUNTER( 9, "master_continue(), error message received" );
		return CONTROL_PROG_OK;
		ZEPTO_DEBUG_ASSERT(0);
	}

	ps->chain_id[0] = zepto_parse_encoded_uint16( received );
	ps->chain_id[1] = zepto_parse_encoded_uint16( received );
	ps->chain_ini_size = zepto_parse_encoded_uint16( received );
	ps->reply_to_id = zepto_parse_encoded_uint16( received );
	ps->self_id = zepto_parse_encoded_uint16( received );
	char tail[256];
//	uint16_t tail_sz = zepto_parsing_remaining_bytes( received );
	uint8_t tail_sz = zepto_parse_uint8( received );
	zepto_parse_read_block( received, (uint8_t*)tail, tail_sz );
	tail[ tail_sz ] = 0;

	// print packet
//	PRINTF( "Yocto: Packet received: [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1], ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );
	ZEPTO_DEBUG_PRINTF_5( "Zepto-Master: Packet received    : [%d bytes]  [%d][0x%04x][0x%04x]",  msg_size, ps->first_byte, ps->chain_id[0], ps->chain_id[1] );
	ZEPTO_DEBUG_PRINTF_5( "[0x%04x][0x%04x][0x%04x]%s\n", ps->chain_ini_size, ps->reply_to_id, ps->self_id, tail );

	// test and analyze

	// size
	if ( !( msg_size >= 7 && msg_size <= 22 ) )
		ZEPTO_DEBUG_PRINTF_2( "ZEPTO: BAD PACKET RECEIVED\n", msg_size );
	ZEPTO_DEBUG_ASSERT( msg_size >= 7 && msg_size <= 22 );

	// flags
	ZEPTO_DEBUG_ASSERT( (ps->first_byte & 4 ) == 0 );
	ps->first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits

	if ( ps->first_byte == SAGDP_P_STATUS_FIRST )
	{
		ZEPTO_DEBUG_ASSERT( 0 == ps->reply_to_id );
		ZEPTO_DEBUG_ASSERT( ps->chain_id[0] != ps->currChainID[0] || ps->chain_id[1] != ps->currChainID[1] );
		ps->currChainID[0] = ps->chain_id[0];
		ps->currChainID[1] = ps->chain_id[1];
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( ps->last_sent_id == ps->reply_to_id );
		ZEPTO_DEBUG_ASSERT( ps->chain_id[0] == ps->currChainID[0] && ps->chain_id[1] == ps->currChainID[1] );
	}


	if ( ps->first_byte == SAGDP_P_STATUS_TERMINATING )
	{
//		chainContinued = false;
		(ps->currChainIdBase[0]) ++;
		INCREMENT_COUNTER( 6, "master_continue(), packet terminating received" );
		return CONTROL_PROG_OK;
	}

	// fake implementation: should this packet be terminal?
	if ( ps->chain_ini_size == ps->self_id + 1 )
		ps->first_byte = SAGDP_P_STATUS_TERMINATING;
	else
		ps->first_byte = SAGDP_P_STATUS_INTERMEDIATE;

	// prepare outgoing packet
	ps->reply_to_id = ps->self_id;
	ps->self_id++;
	ps->last_sent_id = ps->self_id;

	return CONTROL_PROG_CONTINUE;

#if 0
	// scenario decision
//	if ( 0 )
	{
		// just go through
//		*wait_to_process_time = 0;
		return default_test_control_program_accept_reply_continue( control_prog_state, reply );
	}
/*	else
	{
		// request to wait
		*wait_to_process_time = tester_get_rand_val() % 5;
		return CONTROL_PROG_WAIT_TO_CONTINUE;
	}*/
#else
//	return default_test_control_program_accept_reply_continue( control_prog_state, reply );
#endif
}

uint8_t default_test_control_program_accept_reply( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id, void* control_prog_state )
{
	parser_obj po;
	zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
	uint8_t first_byte = zepto_parse_uint8( &po );
	uint16_t frame_sz = zepto_parsing_remaining_bytes( &po );
	return _default_test_control_program_accept_reply( control_prog_state, first_byte & 7/*SAGDP_P_STATUS_MASK*/, &po, frame_sz );
}


#endif // MASTER_ENABLE_ALT_TEST_MODE
