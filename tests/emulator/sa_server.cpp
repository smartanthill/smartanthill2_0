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


#include "sa-common.h"
#include "sa-commlayer.h"
#include "sa-timer.h"
#include "sasp_protocol.h"
#include "sagdp_protocol.h"
#include "yoctovm_protocol.h"
#include "test-generator.h"
#include <stdio.h> 


#define BUF_SIZE 512
uint8_t data_buff[BUF_SIZE];
//uint8_t msgLastSent[BUF_SIZE];
uint8_t pid[ SASP_NONCE_SIZE ];
uint8_t nonce[ SASP_NONCE_SIZE ];









int main_loop()
{
#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	printf("STARTING SERVER...\n");
	printf("==================\n\n");

	tester_initTestSystem();


	// in this preliminary implementation all memory segments are kept separately
	// All memory objects are listed below
	// TODO: revise memory management
	uint8_t miscBuff[BUF_SIZE];
	uint8_t* stack = miscBuff; // first two bytes are used for sizeInOut
	int stackSize = BUF_SIZE / 4 - 2;
	uint8_t timer_val;
	uint16_t wake_time;
	// TODO: revise time/timer management

	uint8_t ret_code;

	// test setup values
	bool wait_for_incoming_chain_with_timer;
	uint16_t wake_time_to_start_new_chain;

	// do necessary initialization
	sagdp_init( data_buff + DADA_OFFSET_SAGDP );


	printf("\nAwaiting client connection... \n" );
	if (!communicationInitializeAsServer())
		return -1;

	printf("Client connected.\n");

	// MAIN LOOP
	for (;;)
	{
getmsg:
		// 1. Get message from comm peer
		ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		INCREMENT_COUNTER_IF( 91, "MAIN LOOP, packet received [1]", ret_code == COMMLAYER_RET_OK );
		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			waitForTimeQuantum();
			if ( timer_val && getTime() >= wake_time )
			{
				printf( "no reply received; the last message (if any) will be resent by timer\n" );
//				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handlerSASP_get_nonce( nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
//					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
			else if ( wait_for_incoming_chain_with_timer && getTime() >= wake_time_to_start_new_chain )
			{
				wait_for_incoming_chain_with_timer = false;
				ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto alt_entry;
				break;
			}
			ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER_IF( 92, "MAIN LOOP, packet received [2]", ret_code == COMMLAYER_RET_OK );
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communicationInitializeAsClient()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}
		printf("Message from client received\n");
		printf( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

//rectosasp:
		// 2. Pass to SASP
		ret_code = handlerSASP_receive( pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP1:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SASP_RET_IGNORE:
			{
				printf( "BAD MESSAGE_RECEIVED\n" );
				goto getmsg;
				break;
			}
			case SASP_RET_TO_LOWER_ERROR:
			{
				goto sendmsg;
				break;
			}
			case SASP_RET_TO_HIGHER_NEW:
			{
				// regular processing will be done below in the next block
				break;
			}
/*			case SASP_RET_TO_HIGHER_REPEATED:
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				ret_code = handlerSAGDP_receiveRepeatedUP( &timer_val, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}*/
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED:
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
//				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handlerSASP_get_nonce( nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
//					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
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

		// 3. pass to SAGDP a new packet
//		ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
		if ( ret_code == SAGDP_RET_START_OVER_FIRST_RECEIVED )
		{
			sagdp_init( data_buff + DADA_OFFSET_SAGDP );
			// TODO: do remaining reinitialization
//			ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
			assert( ret_code != SAGDP_RET_START_OVER_FIRST_RECEIVED );
		}
		else if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handlerSASP_get_nonce( nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
			assert( ret_code == SASP_RET_NONCE );
//			ret_code = handlerSAGDP_receiveUP( &timer_val, nonce, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			ret_code = handlerSAGDP_receiveUP( &timer_val, nonce, pid, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SAGDP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
#ifdef USED_AS_MASTER
			case SAGDP_RET_OK:
			{
				printf( "master received unexpected packet. ignored\n" );
				goto getmsg;
				break;
			}
#else
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinitialize all
				sagdp_init( data_buff + DADA_OFFSET_SAGDP );
//				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
#endif
			case SAGDP_RET_TO_HIGHER:
			{
				// regular processing will be done below in the next block
				break;
			}
			case SAGDP_RET_TO_LOWER_REPEATED:
			{
				goto saspsend;
			}
			case SAGDP_RET_OK:
			{
				goto getmsg;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

processcmd:
		// 4. Process received command (yoctovm)
		ret_code = slave_process( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
/*		if ( ret_code == YOCTOVM_RESET_STACK )
		{
			sagdp_init( data_buff + DADA_OFFSET_SAGDP );
			printf( "slave_process(): ret_code = YOCTOVM_RESET_STACK\n" );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
		}*/
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "YOCTO:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case YOCTOVM_PASS_LOWER:
			{
				 // test generation: sometimes slave can start a new chain at not in-chain reason (although in this case it won't be accepted by Master)
//				bool restart_chain = tester_get_rand_val() % 8 == 0;
				bool restart_chain = false;
				if ( restart_chain )
				{
					sagdp_init( data_buff + DADA_OFFSET_SAGDP );
					ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == YOCTOVM_PASS_LOWER );
				}
				// regular processing will be done below in the next block
				break;
			}
			case YOCTOVM_PASS_LOWER_THEN_IDLE:
			{
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wait_for_incoming_chain_with_timer = true;
				break;
			}
			case YOCTOVM_FAILED:
				sagdp_init( data_buff + DADA_OFFSET_SAGDP );
				// NOTE: no 'break' is here as the rest is the same as for YOCTOVM_OK
			case YOCTOVM_OK:
			{
				// here, in general, two main options are present: 
				// (1) to start a new chain immediately, or
				// (2) to wait, during certain period of time, for an incoming chain, and then, if no packet is received, to start a new chain
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				if ( start_now )
				{
					PRINTF( "   ===  YOCTOVM_OK, forced chain restart  ===\n" );
					ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == YOCTOVM_PASS_LOWER );
					// one more trick: wait for some time to ensure that master will start its own chain, and then send "our own" chain start
/*					bool mutual = tester_get_rand_val() % 5 == 0;
					if ( mutual )
						justWait( 4 );*/
				}
				else
				{
					PRINTF( "   ===  YOCTOVM_OK, delayed chain restart  ===\n" );
					wake_time_to_start_new_chain = getTime() + tester_get_rand_val() % 8;
					wait_for_incoming_chain_with_timer = true;
					goto getmsg;
				}
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

			
			
		// 5. SAGDP
alt_entry:
		uint8_t timer_val;
//		ret_code = handlerSAGDP_receiveHLP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		ret_code = handlerSAGDP_receiveHLP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handlerSASP_get_nonce( nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
			assert( ret_code == SASP_RET_NONCE );
//			ret_code = handlerSAGDP_receiveHLP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			ret_code = handlerSAGDP_receiveHLP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SAGDP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SAGDP_RET_TO_LOWER_NEW:
			{
				// regular processing will be done below in the next block
				// set timer
				break;
			}
			case SAGDP_RET_OK: // TODO: is it possible here?
			{
				goto getmsg;
				break;
			}
			case SAGDP_RET_TO_LOWER_REPEATED: // TODO: is it possible here?
			{
				goto getmsg;
				break;
			}
			case SAGDP_RET_SYS_CORRUPTED: // TODO: is it possible here?
			{
				// TODO: process reset
				sagdp_init( data_buff + DADA_OFFSET_SAGDP );
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wait_for_incoming_chain_with_timer = true;
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
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

		// SASP
saspsend:
		ret_code = handlerSASP_send( nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP2:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SASP_RET_TO_LOWER_REGULAR:
			{
				// regular processing will be done below in the next block
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

sendmsg:
			ret_code = sendMessage( MEMORY_HANDLE_MAIN_LOOP );
			if (ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER( 90, "MAIN LOOP, packet sent" );
			printf("\nMessage replied to client\n");

	}

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();

	return main_loop();
}
