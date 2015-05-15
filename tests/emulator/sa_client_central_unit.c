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


#include "sa-common.h"
#include "sa-commlayer.h"
#include "saccp_protocol.h"
#include "sa_test_control_prog.h"
#include "test-generator.h"
#include <stdio.h> 


int main_loop()
{
#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	tester_initTestSystem();


#if MODEL_IN_EFFECT == 2
	DefaultTestingControlProgramState control_prog_state;
	default_test_control_program_init( &control_prog_state );
#endif

	uint8_t ret_code;

	// test setup values
	bool wait_for_incoming_chain_with_timer = false;
	uint16_t wake_time_to_start_new_chain;

	uint8_t wait_to_continue_processing = 0;
	uint16_t wake_time_continue_processing;


	// Try to initialize connection 
	if ( !communication_initialize() )
		return -1;



	ret_code = default_test_control_program_start_new( &control_prog_state, MEMORY_HANDLE_MAIN_LOOP );
	zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
	goto send_command;

	// MAIN LOOP
	for (;;)
	{
wait_for_comm_event:
		ret_code = wait_for_communication_event( MEMORY_HANDLE_MAIN_LOOP, 1000 ); // TODO: recalculation
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_COMMM_STACK:
			{
				// regular processing will be done below in the next block
				goto process_reply;
				break;
			}
			case COMMLAYER_RET_TIMEOUT:
			{
				// regular processing will be done below in the next block
				printf( "just waiting...\n" );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto wait_for_comm_event;
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

/*start_new_chain:
		ret_code = default_test_control_program_start_new( &control_prog_state, MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		goto send_command;*/

		// 4. Process received command (yoctovm)
process_reply:
		ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP, /*sasp_nonce_type chain_id*/NULL, &control_prog_state ); //master_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		switch ( ret_code )
		{
			case SACCP_RET_CHAIN_DONE:
			{
				ret_code = handler_sacpp_start_new_chain( MEMORY_HANDLE_MAIN_LOOP, &control_prog_state );
				break;
			}
			case SACCP_RET_CHAIN_CONTINUED:
			{
				ret_code = handler_sacpp_continue_chain( MEMORY_HANDLE_MAIN_LOOP, &control_prog_state );
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

	send_command:
		printf( "=============================================Msg is about to be sent; rq_size: %d, rsp_size: %d\n", ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );
		send_to_commm_stack( MEMORY_HANDLE_MAIN_LOOP );
	}

	communication_terminate();

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();

	return main_loop();
}
