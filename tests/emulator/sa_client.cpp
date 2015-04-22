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

		
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	tester_initTestSystem();
/*	bool run_simultaniously = true;
	if ( run_simultaniously )
	{
		requestSyncExec();
	}*/

	// TODO: actual key loading, etc
	uint8_t sasp_key[16];
	memcpy( sasp_key, "16-byte fake key", 16 );

	// in this preliminary implementation all memory segments are kept separately
	// All memory objects are listed below
	// TODO: revise memory management
	uint8_t miscBuff[BUF_SIZE];
	uint8_t* stack = miscBuff; // first two bytes are used for sizeInOut
	uint16_t stackSize = BUF_SIZE / 4 - 2;
	uint8_t timer_val = 0;
	uint16_t wake_time;
	// TODO: revise time/timer management

	uint8_t ret_code;

	// test setup values
	bool wait_for_incoming_chain_with_timer = false;
	uint16_t wake_time_to_start_new_chain;

	uint8_t wait_to_continue_processing = 0;
	uint16_t wake_time_continue_processing;

	// do necessary initialization
	sagdp_init( data_buff + DADA_OFFSET_SAGDP );
	SASP_initAtLifeStart( data_buff + DADA_OFFSET_SASP );

	// Try to open a named pipe; wait for it, if necessary. 
	if ( !communicationInitializeAsClient() )
		return -1;




	ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP );
	zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
	goto entry;

	// MAIN LOOP
	for (;;)
	{
getmsg:
		if ( wait_to_continue_processing && getTime() >= wake_time_continue_processing )
		{
printf( "Processing continued...\n" );
			INCREMENT_COUNTER( 98, "MAIN LOOP, continuing processing" );
			wait_to_continue_processing = 0;
#ifdef USED_AS_MASTER
			ret_code = master_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#else
			ret_code = slave_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#endif
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			goto entry;
			break;
		}

		// 1. Get message from comm peer
		printf("Waiting for a packet from server...\n");
//		ret_code = getMessage( sizeInOut, rwBuff, BUF_SIZE);
		if ( tester_shouldInsertIncomingPacket( MEMORY_HANDLE_MAIN_LOOP ) )
		{
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER( 87, "MAIN LOOP, incoming packet inserted" );
			ret_code = COMMLAYER_RET_OK;
		}
		else
		{
			ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP ); 
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER_IF( 96, "MAIN LOOP, packet received [1]", ret_code == COMMLAYER_RET_OK );
		}

		// sync sending/receiving
		if ( ret_code == COMMLAYER_RET_OK )
		{
			if ( tester_shouldDropIncomingPacket() )
			{
				INCREMENT_COUNTER( 88, "MAIN LOOP, incoming packet dropped [1]" );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto getmsg;
			}

/*+*/			if ( tester_releaseOutgoingPacket( MEMORY_HANDLE_TEST_SUPPORT ) ) // we are on the safe side here as buffer out is not yet used
			{
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
				INCREMENT_COUNTER( 89, "MAIN LOOP, packet sent sync with receiving [1]" );
				sendMessage( MEMORY_HANDLE_TEST_SUPPORT );
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
				assert( ugly_hook_get_request_size( MEMORY_HANDLE_TEST_SUPPORT ) == 0 && ugly_hook_get_response_size( MEMORY_HANDLE_TEST_SUPPORT ) == 0 );
			}/*+*/
		}

		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			waitForTimeQuantum();
			if ( wait_to_continue_processing && getTime() >= wake_time_continue_processing )
			{
printf( "Processing continued...\n" );
				INCREMENT_COUNTER( 98, "MAIN LOOP, continuing processing" );
				wait_to_continue_processing = 0;
#ifdef USED_AS_MASTER
				ret_code = master_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#else
				ret_code = slave_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#endif
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto entry;
				break;
			}
			if ( timer_val != 0 && getTime() >= wake_time )
			{
				printf( "no reply received; the last message (if any) will be resent by timer\n" );
				// TODO: to think: why do we use here handlerSAGDP_receiveRequestResendLSP() and not handlerSAGDP_timer()
//				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				wake_time = 0;
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
//					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				// sync sending/receiving: release presently hold packet (if any), and cause this packet to be hold
				{
					uint16_t sz;
/*+*/					if ( tester_releaseOutgoingPacket( MEMORY_HANDLE_TEST_SUPPORT ) ) // we are on the safe side here as buffer out is not yet used
					{
						zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
						sendMessage( MEMORY_HANDLE_TEST_SUPPORT );
						zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
						tester_requestHoldingPacket();
						INCREMENT_COUNTER( 90, "MAIN LOOP, packet sent sync with receiving [2] (because of timer)" );
					}/*+*/
				}

				goto saspsend;
				break;
			}
			else if ( wait_for_incoming_chain_with_timer && getTime() >= wake_time_to_start_new_chain )
			{
				INCREMENT_COUNTER( 91, "MAIN LOOP, waiting for incoming chain by timer done; starting own chain" );
				wait_for_incoming_chain_with_timer = false;
				ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto entry;
				break;
			}
			ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER_IF( 97, "MAIN LOOP, packet received [2]", ret_code == COMMLAYER_RET_OK );

			// sync sending/receiving
			if ( ret_code == COMMLAYER_RET_OK )
			{
/*+*/				if ( tester_releaseOutgoingPacket( MEMORY_HANDLE_TEST_SUPPORT ) ) // we are on the safe side here as buffer out is not yet used
				{
					INCREMENT_COUNTER( 93, "MAIN LOOP, packet sent sync with receiving [3]" );
					zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
					sendMessage( MEMORY_HANDLE_TEST_SUPPORT );
					zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
				}/*+*/

				if ( tester_shouldDropIncomingPacket() )
				{
					INCREMENT_COUNTER( 92, "MAIN LOOP, incoming packet dropped [2]" );
					goto getmsg;
				}
			}
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communicationInitializeAsClient()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}

		tester_registerIncomingPacket( MEMORY_HANDLE_MAIN_LOOP );
		printf("Message from server received\n");
		printf( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );



rectosasp:
		// 2. Pass to SASP
		ret_code = handler_sasp_receive( sasp_key, pid, MEMORY_HANDLE_MAIN_LOOP, data_buff + DADA_OFFSET_SASP );
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
				ret_code = handlerSAGDP_receiveRepeatedUP( &timer_val, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
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
				else if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, data_buff + DADA_OFFSET_SASP );
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
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, data_buff + DADA_OFFSET_SASP );
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
				goto getmsg;
				break;
			}
#endif
			case SAGDP_RET_TO_HIGHER:
			{
				// regular processing will be done below, but we need to jump over 
				break;
			}
			case SAGDP_RET_TO_HIGHER_ERROR:
			{
				sagdp_init( data_buff + DADA_OFFSET_SAGDP );
				// TODO: reinit the rest of stack (where applicable)
				ret_code = master_error( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto entry;
				break;
			}
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

processcmd:
		// 4. Process received command (yoctovm)
		ret_code = master_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
/*		if ( ret_code == YOCTOVM_RESET_STACK )
		{
			sagdp_init( data_buff + DADA_OFFSET_SAGDP );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
		}*/
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
entry:	
		wait_for_incoming_chain_with_timer = false;
		if ( ret_code == YOCTOVM_WAIT_TO_CONTINUE )
			printf( "YOCTO:  ret: %d; waiting to continue ...\n", ret_code );
		else
			printf( "YOCTO:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case YOCTOVM_PASS_LOWER:
			{
				 // test generation: sometimes master can start a new chain at not in-chain reason
				bool restart_chain = tester_get_rand_val() % 8 == 0;
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
				bool start_now = tester_get_rand_val() % 3 == 0;
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
//				bool start_now = get_rand_val() % 3 == 0;
				bool start_now = true;
				if ( start_now )
				{
					PRINTF( "   ===  YOCTOVM_OK, forced chain restart  ===\n" );
					ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4, stack, stackSize*/ );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == YOCTOVM_PASS_LOWER );
					// one more trick: wait for some time to ensure that slave will start its own chain, and then send "our own" chain start
/*					bool mutual = get_rand_val() % 5 == 0;
					if ( mutual )
						justWait( 5 );*/
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
			case YOCTOVM_WAIT_TO_CONTINUE:
			{
				if ( wait_to_continue_processing == 0 ) wait_to_continue_processing = 1;
				wake_time_continue_processing = getTime() + wait_to_continue_processing;
printf( "Processing in progress... (period = %d, time = %d)\n", wait_to_continue_processing, wake_time_continue_processing );
				goto getmsg;
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
//		ret_code = handlerSAGDP_receiveHLP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		ret_code = handlerSAGDP_receiveHLP( &timer_val, NULL, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, data_buff + DADA_OFFSET_SASP );
			assert( ret_code == SASP_RET_NONCE );
//			ret_code = handlerSAGDP_receiveHLP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			ret_code = handlerSAGDP_receiveHLP( &timer_val, nonce, MEMORY_HANDLE_MAIN_LOOP, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
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
		ret_code = handler_sasp_send( sasp_key, nonce, MEMORY_HANDLE_MAIN_LOOP, data_buff + DADA_OFFSET_SASP );
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

	sendmsg:
//		allowSyncExec();
		tester_registerOutgoingPacket( MEMORY_HANDLE_MAIN_LOOP );

/*+*/		bool syncSendReceive;
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
		else/*+*/
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

	communicationTerminate();

	return 0;
}

#define USE_128

#include "sa-aes-128.h"


#if 0
#include <sys/types.h>
#ifdef _MSC_VER
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 int main_CL(void)
  {
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (-1 == SocketFD)
    {
      perror("cannot create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(stSockAddr));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(1100);
    Res = inet_pton(AF_INET, "192.168.1.3", &stSockAddr.sin_addr);
 
    if (0 > Res)
    {
      perror("error: first parameter is not a valid address family");
//      close(SocketFD);
      closesocket(SocketFD);
      exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
      perror("char string (second parameter does not contain valid ipaddress)");
//      close(SocketFD);
      closesocket(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
      perror("connect failed");
//      close(SocketFD);
      closesocket(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    /* perform read write operations ... */
 
//    (void) shutdown(SocketFD, SHUT_RDWR);
    (void) shutdown(SocketFD, SD_BOTH);
 
//    close(SocketFD);
      closesocket(SocketFD);
    return EXIT_SUCCESS;
  }
#endif // 0

#include "sa-unit-tests-aes-eax.h"
int main(int argc, char *argv[])
{
//	test_aes_and_eax_128(); return 0;
	
	zepto_mem_man_init_memory_management();

	return main_loop();
}
