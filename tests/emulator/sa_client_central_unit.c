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


//#define MODEL_IN_EFFECT 1
#define MODEL_IN_EFFECT 2


#include "../../firmware/src/common/sa_common.h"
#include "../../firmware/src/hal/hal_commlayer.h"
//#include "saccp_protocol.h"
#include "sa_test_control_prog.h"
#include "test_generator.h"
#include "../../firmware/src/common/zepto_mem_mngmt.h"
#include "saccp_protocol_client_side_cu.h"

#include <stdio.h>


int main_loop()
{
#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	ZEPTO_DEBUG_PRINTF_1("starting CLIENT...\n");
	ZEPTO_DEBUG_PRINTF_1("==================\n\n");

	tester_initTestSystem();


	uint8_t ret_code;

	// test setup values
//	bool wait_for_incoming_chain_with_timer = false;
//	uint16_t wake_time_to_start_new_chain;

	uint8_t wait_to_continue_processing = 0;
//	uint16_t wake_time_continue_processing;


	// Try to initialize connection
	bool comm_init_ok = communication_initialize();
	if ( !comm_init_ok )
	{
		return -1;
	}



#ifdef MASTER_ENABLE_ALT_TEST_MODE

#if MODEL_IN_EFFECT == 2
	extern DefaultTestingControlProgramState DefaultTestingControlProgramState_struct;
	default_test_control_program_init( &DefaultTestingControlProgramState_struct );
#endif
	ret_code = default_test_control_program_start_new( &DefaultTestingControlProgramState_struct, MEMORY_HANDLE_MAIN_LOOP_1 );
	zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
	handler_saccp_prepare_to_send( MEMORY_HANDLE_MAIN_LOOP_1 );
	zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
/*	parser_obj po;
	zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP_1 );
	uint8_t bu[40];
	memset( bu, 0, 40 );
	zepto_parse_read_block( &po, bu, zepto_parsing_remaining_bytes( &po ) );
	for ( int k=0;k<30; k++)
		ZEPTO_DEBUG_PRINTF_3( "%c [0x%x]\n", bu[k], bu[k] );
	return 0;*/
#else
	uint8_t buff_base[] = {0x2, 0x0, 0x8, 0x1, 0x1, 0x2, 0x0, 0x1, '-', '-', '>' }; 
	uint8_t buff[128];
	buff[0] = 1; // first in the chain
	memcpy( buff+1, buff_base, sizeof(buff_base) );
	zepto_write_block( MEMORY_HANDLE_MAIN_LOOP_1, buff, 1+sizeof(buff_base) );
	zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
	// should appear on the other side: Packet received: [8 bytes]  [1][0x0001][0x0001][0x0002][0x0000][0x0001]-->
	// should come back: 02 01 01 02 01 02 2d 2d 2d 2d 3e
#endif
	goto send_command;

	// MAIN LOOP
	for (;;)
	{
wait_for_comm_event:
		ret_code = wait_for_communication_event( 200 ); // TODO: recalculation
//		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );

		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_COMMM_STACK:
			{
				// regular processing will be done below in the next block
				ret_code = try_get_message_within_master( MEMORY_HANDLE_MAIN_LOOP_1 );
				if ( ret_code == COMMLAYER_RET_FAILED )
					return 0;
				ZEPTO_DEBUG_ASSERT( ret_code == COMMLAYER_RET_OK );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				ZEPTO_DEBUG_PRINTF_3( "msg received; rq_size: %d, rsp_size: %d\n", ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );
				goto process_reply;
				break;
			}
			case COMMLAYER_RET_TIMEOUT:
			{
				// regular processing will be done below in the next block
//				ZEPTO_DEBUG_PRINTF_1( "just waiting...\n" );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				goto wait_for_comm_event;
				break;
			}
			default:
			{
				// unexpected ret_code
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d received from wait_for_communication_event()\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				return 0;
				break;
			}
		}

/*start_new_chain:
		ret_code = default_test_control_program_start_new( &DefaultTestingControlProgramState_struct, MEMORY_HANDLE_MAIN_LOOP_1 );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		goto send_command;*/

		// 4. Process received command (yoctovm)
#ifdef MASTER_ENABLE_ALT_TEST_MODE
	process_reply:
#if 1
		// do reply preprocessing
		ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP_1, /*sasp_nonce_type chain_id*/NULL, &DefaultTestingControlProgramState_struct );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		switch ( ret_code )
		{
			case CONTROL_PROG_PASS_LOWER_THEN_IDLE:
			{
				ret_code = default_test_control_program_start_new( &DefaultTestingControlProgramState_struct, MEMORY_HANDLE_MAIN_LOOP_1 );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				handler_saccp_prepare_to_send( MEMORY_HANDLE_MAIN_LOOP_1 );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				break;
			}
			case CONTROL_PROG_PASS_LOWER:
			{
				break;
			}
			default:
			{
				// unexpected ret_code
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}
#else
		ret_code = default_test_control_program_accept_reply( MEMORY_HANDLE_MAIN_LOOP_1, /*sasp_nonce_type chain_id*/NULL, &DefaultTestingControlProgramState_struct );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		switch ( ret_code )
		{
			case CONTROL_PROG_OK:
			{
//				ret_code = handler_sacpp_start_new_chain( MEMORY_HANDLE_MAIN_LOOP_1, &DefaultTestingControlProgramState_struct );
				ret_code = default_test_control_program_start_new( &DefaultTestingControlProgramState_struct, MEMORY_HANDLE_MAIN_LOOP_1 );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				break;
			}
			case CONTROL_PROG_CONTINUE:
			{
//				ret_code = handler_sacpp_continue_chain( MEMORY_HANDLE_MAIN_LOOP_1, &DefaultTestingControlProgramState_struct );
				ret_code = default_test_control_program_accept_reply_continue( &DefaultTestingControlProgramState_struct, MEMORY_HANDLE_MAIN_LOOP_1 );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				break;
			}
			default:
			{
				// unexpected ret_code
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}
#endif
#else // MASTER_ENABLE_ALT_TEST_MODE
		// so far just print received packet and exit
		uint8_t buff[128];
		uint16_t i;
		parser_obj po;
process_reply:
		zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP_1 );
		uint16_t sz = zepto_parsing_remaining_bytes( &po );
		zepto_parse_read_block( &po, buff, sz );
		ZEPTO_DEBUG_PRINTF_1( "block received:\n" );
		for ( i=0; i<sz; i++ )
			ZEPTO_DEBUG_PRINTF_2( "%02x ", buff[i] );
		ZEPTO_DEBUG_PRINTF_1( "\n\n" );

#endif // MASTER_ENABLE_ALT_TEST_MODE

send_command:
		ZEPTO_DEBUG_PRINTF_3( "=============================================Msg is about to be sent; rq_size: %d, rsp_size: %d\n", ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );
		send_to_commm_stack( MEMORY_HANDLE_MAIN_LOOP_1 );
	}

	communication_terminate();

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();

	return main_loop();
}
