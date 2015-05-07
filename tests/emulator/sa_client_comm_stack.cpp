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
#include "sa-timer.h"
#include "saoudp_protocol.h"
#include "sasp_protocol.h"
#include "sagdp_protocol.h"
#include "test-generator.h"
#include <stdio.h> 

uint8_t pid[ SASP_NONCE_SIZE ];
uint8_t nonce[ SASP_NONCE_SIZE ];

/*
uint8_t send_to_central_unit_error( MEMORY_HANDLE mem_h )
{
	return send_to_central_unit( mem_h );
}

uint8_t send_to_central_unit_reply( MEMORY_HANDLE mem_h )
{
	return send_to_central_unit( mem_h );
}
*/

int main_loop()
{
#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	printf("starting CLIENT's COMMM STACK...\n");
	printf("================================\n\n");

	tester_initTestSystem();

	// TODO: actual key loading, etc
	uint8_t sasp_key[16];
	memcpy( sasp_key, "16-byte fake key", 16 );

	uint8_t timer_val = 0xFF;
	uint16_t wake_time;
	// TODO: revise time/timer management

	uint8_t ret_code;

	uint8_t wait_to_continue_processing = 0;
	uint16_t wake_time_continue_processing;

	// do necessary initialization
	SAGDP_DATA sagdp_data;
	sagdp_init( &sagdp_data );
	SASP_DATA sasp_data;
	SASP_initAtLifeStart( &sasp_data );

	// Try to initialize connection 
	if ( !communication_initialize() )
		return -1;


	// MAIN LOOP
	for (;;)
	{
wait_for_comm_event:
		ret_code = wait_for_communication_event( MEMORY_HANDLE_MAIN_LOOP, timer_val*100 ); // TODO: recalculation
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_CENTRAL_UNIT:
			{
				// regular processing will be done below in the next block
				goto client_received;
				break;
			}
			case COMMLAYER_RET_FROM_DEV:
			{
				// regular processing will be done below in the next block
				goto saoudp_in;
				break;
			}
			case COMMLAYER_RET_TIMEOUT:
			{
				// regular processing will be done below in the next block
				printf( "no reply from device received; the last message (if any) will be resent by timer\n" );
				// TODO: to think: why do we use here handler_sagdp_receive_request_resend_lsp() and not handlerSAGDP_timer()
				ret_code = handler_sagdp_receive_request_resend_lsp( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				wake_time = 0;
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, &sasp_data );
					assert( ret_code == SASP_RET_NONCE );
					ret_code = handler_sagdp_receive_request_resend_lsp( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
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


		tester_registerIncomingPacket( MEMORY_HANDLE_MAIN_LOOP );
		printf("Message from server received\n");
		printf( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );


		// 2.1. Pass to SAoUDP
saoudp_in:
		ret_code = handler_saoudp_receive( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case SAOUDP_RET_OK:
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

		// 2.2. Pass to SASP
		ret_code = handler_sasp_receive( sasp_key, pid, MEMORY_HANDLE_MAIN_LOOP, &sasp_data );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP1:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SASP_RET_IGNORE:
			{
				printf( "BAD MESSAGE_RECEIVED\n" );
				goto wait_for_comm_event;
				break;
			}
			case SASP_RET_TO_LOWER_ERROR:
			{
				goto saoudp_send;
				break;
			}
			case SASP_RET_TO_HIGHER_NEW:
			{
				// regular processing will be done below in the next block
				break;
			}
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED:
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				ret_code = handler_sagdp_receive_request_resend_lsp( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				else if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, &sasp_data );
					assert( ret_code == SASP_RET_NONCE );
					ret_code = handler_sagdp_receive_request_resend_lsp( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
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
		ret_code = handler_sagdp_receive_up( &timer_val, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, &sasp_data );
			assert( ret_code == SASP_RET_NONCE );
			ret_code = handler_sagdp_receive_up( &timer_val, nonce, pid, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
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
				goto wait_for_comm_event;
				break;
			}
#else
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinitialize all
				goto wait_for_comm_event;
				break;
			}
#endif
			case SAGDP_RET_TO_HIGHER:
			{
				// regular processing will be done below, but we need to jump over 
				break;
			}
#if 0
			case SAGDP_RET_TO_HIGHER_ERROR:
			{
				sagdp_init( &sagdp_data );
				// TODO: reinit the rest of stack (where applicable)
				ret_code = send_to_central_unit_error( MEMORY_HANDLE_MAIN_LOOP );
				//+++TODO: where to go?
				goto wait_for_comm_event;
				break;
			}
#endif // 0
			case SAGDP_RET_TO_LOWER_REPEATED:
			{
				goto saspsend;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

		ret_code = send_to_central_unit( MEMORY_HANDLE_MAIN_LOOP );
		goto wait_for_comm_event;
		//+++ TODO: what? goto select?
			
		// 5. SAGDP
client_received:
		ret_code = handler_sagdp_receive_hlp( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, &sasp_data );
			assert( ret_code == SASP_RET_NONCE );
			ret_code = handler_sagdp_receive_hlp( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SAGDP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinit the system
				PRINTF( "Internal error. System is to be reinitialized\n" );
				break;
			}
			case SAGDP_RET_TO_LOWER_NEW:
			{
				// regular processing will be done below in the next block
				wake_time = getTime() + timer_val;
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
		ret_code = handler_sasp_send( sasp_key, nonce, MEMORY_HANDLE_MAIN_LOOP, &sasp_data );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

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

		// SAoUDP
saoudp_send:
		ret_code = handler_saoudp_send( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case SAOUDP_RET_OK:
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
		tester_registerOutgoingPacket( MEMORY_HANDLE_MAIN_LOOP );

		bool syncSendReceive;
		bool is_packet_to_send = true;
		if ( tester_holdPacketOnRequest( MEMORY_HANDLE_MAIN_LOOP ) )
		{
			INCREMENT_COUNTER( 95, "MAIN LOOP, holdPacketOnRequest() called" );
			syncSendReceive = false;
			is_packet_to_send = false;
		}
		else
			syncSendReceive = tester_get_rand_val() % 4 == 0 && !tester_isOutgoingPacketOnHold();

		if ( syncSendReceive )
		{
			INCREMENT_COUNTER( 94, "MAIN LOOP, holdOutgoingPacket() called" );
			tester_holdOutgoingPacket( MEMORY_HANDLE_MAIN_LOOP );
		}
		else
		{
			if ( tester_shouldInsertOutgoingPacket( MEMORY_HANDLE_TEST_SUPPORT ) )
			{
				INCREMENT_COUNTER( 80, "MAIN LOOP, packet inserted" );
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
				sendMessage( MEMORY_HANDLE_TEST_SUPPORT );
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
			}

			if ( !tester_shouldDropOutgoingPacket() )
			{
				if ( is_packet_to_send )
				{
					ret_code = sendMessage( MEMORY_HANDLE_MAIN_LOOP );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					if (ret_code != COMMLAYER_RET_OK )
					{
						return -1;
					}
					INCREMENT_COUNTER( 82, "MAIN LOOP, packet sent" );
					printf("\nMessage sent to comm peer\n");
				}
			}
			else
			{
				INCREMENT_COUNTER( 81, "MAIN LOOP, packet dropped" );
				printf("\nMessage lost on the way...\n");
			}
		}

	}

	communication_terminate();

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();

	return main_loop();
//	return 0;
}
