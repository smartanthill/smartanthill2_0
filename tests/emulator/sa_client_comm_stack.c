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


#include "../../firmware/src/common/sa_common.h"
#include "../../firmware/src/hal/hal_commlayer.h"
#include "../../firmware/src/hal/hal_time_provider.h"
#include "../../firmware/src/common/saoudp_protocol.h"
#include "../../firmware/src/common/sasp_protocol.h"
#include "../../firmware/src/common/sagdp_protocol.h"
#include "saccp_protocol_client_side.h"
#include "test_generator.h"
#include <stdio.h> 
#include "../../firmware/src/zepto_config.h"

DECLARE_AES_ENCRYPTION_KEY


int main_loop()
{
	uint8_t pid[ SASP_NONCE_SIZE ];
	uint8_t nonce[ SASP_NONCE_SIZE ];

#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	ZEPTO_DEBUG_PRINTF_1("starting CLIENT's COMMM STACK...\n");
	ZEPTO_DEBUG_PRINTF_1("================================\n\n");

	tester_initTestSystem();

	// TODO: actual key loading, etc
//	uint8_t AES_ENCRYPTION_KEY[16];
//	memcpy( AES_ENCRYPTION_KEY, "16-byte fake key", 16 );
//	memset( AES_ENCRYPTION_KEY, 0xab, 16 );

//	timeout_action tact;
//	tact.action = 0;
	sa_time_val currt;
	waiting_for wait_for;
	memset( &wait_for, 0, sizeof( waiting_for ) );
	wait_for.wait_packet = 1;
	TIME_MILLISECONDS16_TO_TIMEVAL( 1000, wait_for.wait_time ) //+++TODO: actual processing throughout the code

	uint8_t timer_val = 0xFF;
	uint16_t wake_time;
	// TODO: revise time/timer management

	uint8_t ret_code;

//	uint8_t wait_to_continue_processing = 0;
//	uint16_t wake_time_continue_processing;

	// do necessary initialization
/*	SAGDP_DATA sagdp_data;
	SASP_DATA sasp_data;
	sagdp_init( &sagdp_data );
	sasp_init_at_lifestart( &sasp_data );*/
	sagdp_init();
	sasp_init_at_lifestart();

	// Try to initialize connection 
	if ( !communication_initialize() )
		return -1;

//	REQUEST_REPLY_HANDLE working_handle = MEMORY_HANDLE_MAIN_LOOP_2;
//	REQUEST_REPLY_HANDLE packet_getting_handle = MEMORY_HANDLE_MAIN_LOOP_1;


	// MAIN LOOP
	for (;;)
	{
wait_for_comm_event:
//		ret_code = wait_for_communication_event( MEMORY_HANDLE_MAIN_LOOP_1, timer_val*100 ); // TODO: recalculation
		ret_code = wait_for_communication_event( timer_val*100 ); // TODO: recalculation
//		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "=============================================Msg wait event; ret = %d, rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );

		switch ( ret_code )
		{
			case COMMLAYER_RET_FAILED:
			{
				// regular processing will be done below in the next block
				return 0;
				break;
			}
			case COMMLAYER_RET_FROM_CENTRAL_UNIT:
			{
				// regular processing will be done below in the next block
				ret_code = try_get_message_within_master( MEMORY_HANDLE_MAIN_LOOP_1 );
				if ( ret_code == COMMLAYER_RET_FAILED )
					return 0;
				ZEPTO_DEBUG_ASSERT( ret_code == COMMLAYER_RET_OK );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				goto client_received;
				break;
			}
			case COMMLAYER_RET_FROM_DEV:
			{
				// regular processing will be done below in the next block
				ret_code = hal_get_packet_bytes( MEMORY_HANDLE_MAIN_LOOP_1 );
				if ( ret_code == HAL_GET_PACKET_BYTES_FAILED )
					return 0;
				ZEPTO_DEBUG_ASSERT( ret_code == HAL_GET_PACKET_BYTES_DONE );
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				goto saoudp_in;
				break;
			}
			case COMMLAYER_RET_TIMEOUT:
			{
				// regular processing will be done below in the next block
/*				ZEPTO_DEBUG_PRINTF_1( "no reply from device received; the last message (if any) will be resent by timer\n" );
				// TODO: to think: why do we use here handler_sagdp_receive_request_resend_lsp() and not handlerSAGDP_timer()
				sa_get_time( &currt );
				ret_code = handler_sagdp_timer( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP_1, &sagdp_data );
				if ( ret_code == SAGDP_RET_OK )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
					continue;
				}
				wake_time = 0;
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE, &sasp_data );
					ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
					sa_get_time( &currt );
					ret_code = handler_sagdp_timer( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP_1, &sagdp_data );
					ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_OK );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );

				goto wait_for_comm_event;
				break;*/
//				if ( sagdp_data.event_type ) //TODO: temporary solution
				if (1) //TODO: temporary solution
				{
					ZEPTO_DEBUG_PRINTF_1( "no reply received; the last message (if any) will be resent by timer\n" );
					sa_get_time( &currt );
					ret_code = handler_sagdp_timer( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
					if ( ret_code == SAGDP_RET_OK )
					{
						zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
						goto wait_for_comm_event;
					}
					else if ( ret_code == SAGDP_RET_NEED_NONCE )
					{
						ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
						ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
						sa_get_time( &currt );
						ret_code = handler_sagdp_timer( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
//			ZEPTO_DEBUG_PRINTF_1( "ret_code = %d\n", ret_code );
						ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_OK );
						zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
						goto saspsend;
						break;
					}
					else
					{
						ZEPTO_DEBUG_PRINTF_2( "ret_code = %d\n", ret_code );
						ZEPTO_DEBUG_ASSERT( 0 );
					}
				}
				goto wait_for_comm_event;
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


		tester_registerIncomingPacket( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_1("Message from server received\n");
		ZEPTO_DEBUG_PRINTF_4( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );


		// 2.1. Pass to SAoUDP
saoudp_in:
		ret_code = handler_saoudp_receive( MEMORY_HANDLE_MAIN_LOOP_1 );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );

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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

		// 2.2. Pass to SASP
		ret_code = handler_sasp_receive( AES_ENCRYPTION_KEY, pid, MEMORY_HANDLE_MAIN_LOOP_1/*, &sasp_data*/ );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SASP1:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );

		switch ( ret_code )
		{
			case SASP_RET_IGNORE:
			{
				ZEPTO_DEBUG_PRINTF_1( "BAD MESSAGE_RECEIVED\n" );
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
				ZEPTO_DEBUG_PRINTF_1( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				sa_get_time( &currt );
				ret_code = handler_sagdp_receive_request_resend_lsp( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
					continue;
				}
				else if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
					ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
					sa_get_time( &currt );
					ret_code = handler_sagdp_receive_request_resend_lsp( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
					ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
				goto saspsend;
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

		// 3. pass to SAGDP a new packet
		sa_get_time( &currt );
		ret_code = handler_sagdp_receive_up( &currt, &wait_for, NULL, pid, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
			sa_get_time( &currt );
			ret_code = handler_sagdp_receive_up( &currt, &wait_for, nonce, pid, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SAGDP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );

		switch ( ret_code )
		{
#ifdef USED_AS_MASTER
			case SAGDP_RET_OK:
			{
				ZEPTO_DEBUG_PRINTF_1( "master received unexpected packet. ignored\n" );
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
				ret_code = send_to_central_unit_error( MEMORY_HANDLE_MAIN_LOOP_1 );
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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

#ifdef MASTER_ENABLE_ALT_TEST_MODE
		ret_code = send_to_central_unit( MEMORY_HANDLE_MAIN_LOOP_1 );
		goto wait_for_comm_event;
#else

		// 4. pass to SACCP a new packet
#if 0 // we cannot do any essential processing here in comm stack...
		ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP_1/*, sasp_nonce_type chain_id*/ ); //master_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP_1 );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SACCP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );
		switch ( ret_code )
		{
			case SACCP_RET_PASS_TO_CENTRAL_UNIT:
			{
				ret_code = send_to_central_unit( MEMORY_HANDLE_MAIN_LOOP_1 );
				// TODO: check ret_code
				goto wait_for_comm_event;
				break;
			}
			case SACCP_RET_FAILED:
			{
				ZEPTO_DEBUG_PRINTF_1( "Failure in SACCP. handling is not implemented. Aborting\n" );
				return 0;
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

#else	// ...instead we just send whatever we have received  to the Central Unit.
		// Note: we may need to add some data (such as chain ID) or to somehow restructure the packet data; 
		//       in this case this is a right place to do that

		ret_code = send_to_central_unit( MEMORY_HANDLE_MAIN_LOOP_1 );
		// TODO: check ret_code
		goto wait_for_comm_event;

#endif // 0

#endif			




	client_received:
#if 0 // this functionality is trivial and will be done on a Central Unit side
		// 4. SACCP (prepare packet)
		ret_code = handler_saccp_prepare_to_send( MEMORY_HANDLE_MAIN_LOOP_1 );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SACCP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );
		// TODO: analyze and process ret_code
#endif

		// 5. SAGDP
		ZEPTO_DEBUG_PRINTF_3( "@client_received: rq_size: %d, rsp_size: %d\n", ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );
		sa_get_time( &currt );
		ret_code = handler_sagdp_receive_hlp( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id(  nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
			sa_get_time( &currt );
			ret_code = handler_sagdp_receive_hlp( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP_1/*, &sagdp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SAGDP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );

		switch ( ret_code )
		{
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinit the system
				ZEPTO_DEBUG_PRINTF_1( "Internal error. System is to be reinitialized\n" );
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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

		// SASP
saspsend:
		ret_code = handler_sasp_send( AES_ENCRYPTION_KEY, nonce, MEMORY_HANDLE_MAIN_LOOP_1/*, &sasp_data*/ );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );
		ZEPTO_DEBUG_PRINTF_4( "SASP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP_1 ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP_1 ) );

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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

		// SAoUDP
saoudp_send:
		ret_code = handler_saoudp_send( MEMORY_HANDLE_MAIN_LOOP_1 );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP_1 );

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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

		tester_registerOutgoingPacket( MEMORY_HANDLE_MAIN_LOOP_1 );

		bool syncSendReceive;
		bool is_packet_to_send = true;
		if ( tester_holdPacketOnRequest( MEMORY_HANDLE_MAIN_LOOP_1 ) )
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
			tester_holdOutgoingPacket( MEMORY_HANDLE_MAIN_LOOP_1 );
		}
		else
		{
#ifdef SA_DEBUG
			if ( tester_shouldInsertOutgoingPacket( MEMORY_HANDLE_TEST_SUPPORT ) )
			{
				INCREMENT_COUNTER( 80, "MAIN LOOP, packet inserted" );
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
				send_message( MEMORY_HANDLE_TEST_SUPPORT );
				zepto_response_to_request( MEMORY_HANDLE_TEST_SUPPORT );
			}
#endif

			if ( !tester_shouldDropOutgoingPacket() )
			{
				if ( is_packet_to_send )
				{
					ret_code = send_message( MEMORY_HANDLE_MAIN_LOOP_1 );
					zepto_parser_free_memory( MEMORY_HANDLE_MAIN_LOOP_1 );
					if (ret_code != COMMLAYER_RET_OK )
					{
						return -1;
					}
					INCREMENT_COUNTER( 82, "MAIN LOOP, packet sent" );
					ZEPTO_DEBUG_PRINTF_1("\nMessage sent to comm peer\n");
				}
			}
			else
			{
				INCREMENT_COUNTER( 81, "MAIN LOOP, packet dropped" );
				ZEPTO_DEBUG_PRINTF_1("\nMessage lost on the way...\n");
			}
		}

	}

	communication_terminate();

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();
	if (!init_eeprom_access())
		return 0;
//	format_eeprom_at_lifestart();

	return main_loop();
//	return 0;
}
