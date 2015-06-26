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


#include "common/sa_common.h"
#include "common/sa_uint48.h"
#include "hal/hal_time_provider.h"
#include "hal/hal_commlayer.h"
#include "hal/hal_waiting.h"
#include "common/saoudp_protocol.h"
#include "common/sasp_protocol.h"
#include "common/sagdp_protocol.h"
#include "common/saccp_protocol.h"
#include "plugins/smart_echo/smart_echo.h"
#include "zepto_config.h"



// TODO: actual key loading, etc
//uint8_t AES_ENCRYPTION_KEY[16];
DECLARE_AES_ENCRYPTION_KEY

waiting_for wait_for;

//#define ALLOW_PRINTING_SASP_INCOMING_MESSAGE
//#define ALLOW_PRINTING_SASP_OUTGOING_MESSAGE
//#define TEST_RAM_CONSUMPTION

#ifdef TEST_RAM_CONSUMPTION

const uint8_t INCOMING_TEST_PACKET[] ZEPTO_PROG_CONSTANT_LOCATION =
{
/*	0x02, 0x89, 0x61, 0xd9, 0x1d, 0x54, 0xb4, 0xac,
	0xbb, 0xcb, 0x6c, 0x74, 0x0f, 0x53, 0xbf, 0x7f,
	0x0a, 0x63, 0xac, 0x7d, 0xf8, 0x31, 0x0c, 0xe2,
	0x2f, 0x25, 0xc8, 0x8c, 0x6f, 0xd0, 0x15, 0x53,
	0x79, 0x15, 0xbd, 0x15, 0xf9, 0x8a, 0x10, 0x3c,
	0x73, 0x9e, 0x93, 0xf4,*/
	0x02, 0x89, 0x61, 0xd9, 0x1d, 0x54, 0xb4, 0xac,
	0xbb, 0xcb, 0x6c, 0x74, 0x0c, 0x53, 0xbf, 0x7f,
	0x0a, 0x63, 0xac, 0x7d, 0xd3, 0x31, 0x0c, 0xe2,
	0x2f, 0x36, 0xfe, 0x84, 0x4a, 0xb5, 0xdb, 0x59,
	0xaf, 0xf1, 0x45, 0x08, 0x3c, 0xfa, 0xab, 0x99,
	0x9c, 0x74, 0x68, 0x3c, 0xe5,
};

const uint8_t OUTGOING_TEST_PACKET[] ZEPTO_PROG_CONSTANT_LOCATION =
{
	0x02, 0x88, 0xe1, 0xd6, 0xe3, 0xaa, 0x4a, 0x53,
	0x25, 0x36, 0x6e, 0x7c, 0x03, 0x50, 0x3a, 0xad,
	0x27, 0x46, 0x81, 0x42, 0x14, 0x86, 0x7a, 0xe1,
	0x2c, 0x10, 0xca, 0x1f, 0x25, 0xcb, 0xed, 0xcc,
	0xc0, 0xe2, 0x29, 0xaf,
};

//uint8_t FAKE_ARRAY_UNDER_STACK[1024-291] __attribute__ ((section (".noinit"))); // TODO: 1024 stands for RAM available, and 291 for reported total size of initial allocations for globals/statics
uint8_t FAKE_ARRAY_UNDER_STACK[1024-291];

void StackPaint(void)
{
	volatile int test_byte = 0;
	test_byte++;
	uint8_t *p = FAKE_ARRAY_UNDER_STACK;
	while( p <= FAKE_ARRAY_UNDER_STACK + sizeof(FAKE_ARRAY_UNDER_STACK) - 64 )
	{
		*p = 0xaa;
		p++;
	}
	test_byte++;
}

#endif // TEST_RAM_CONSUMPTION

bool sa_main_init()
{
#ifdef TEST_RAM_CONSUMPTION
	StackPaint();
#endif // TEST_RAM_CONSUMPTION
	zepto_mem_man_init_memory_management();
	if (!init_eeprom_access())
		return false;
//	format_eeprom_at_lifestart();

#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

	ZEPTO_DEBUG_PRINTF_1("STARTING SERVER...\n");
	ZEPTO_DEBUG_PRINTF_1("==================\n\n");

	memset( &wait_for, 0, sizeof( waiting_for ) );
	wait_for.wait_packet = 1;
	TIME_MILLISECONDS16_TO_TIMEVAL( 1000, wait_for.wait_time ); //+++TODO: actual processing throughout the code

//    memset( AES_ENCRYPTION_KEY, 0xab, 16 );
	sasp_init_at_lifestart(); // TODO: replace by more extensive restore-from-backup-etc
	sagdp_init();
	zepto_vm_init();

	ZEPTO_DEBUG_PRINTF_1("\nAwaiting client connection... \n" );
	if (!communication_initialize())
		return false;

	ZEPTO_DEBUG_PRINTF_1("Client connected.\n");

    return true;
}

int sa_main_loop()
{
#ifdef TEST_RAM_CONSUMPTION
	uint8_t j;
	volatile int test_byte = 0;
	test_byte++;
#endif // TEST_RAM_CONSUMPTION

	waiting_for ret_wf;
	uint8_t pid[ SASP_NONCE_SIZE ];
	uint8_t nonce[ SASP_NONCE_SIZE ];
	uint8_t ret_code;
	sa_time_val currt;

	// test setup values
	// TODO: all code related to simulation and test generation MUST be moved out here ASAP!
	bool wait_for_incoming_chain_with_timer = 0;
	uint16_t wake_time_to_start_new_chain = 0;
//	uint8_t wait_to_continue_processing = 0;
//	uint16_t wake_time_continue_processing = 0;
	// END OF test setup values

	for (;;)
	{
		// 1. Get message from comm peer
/*		ret_code = try_get_message( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_ASSERT( ret_code != COMMLAYER_RET_OK || ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ) != 0 );*/
		ret_code = COMMLAYER_RET_PENDING;
		INCREMENT_COUNTER_IF( 91, "MAIN LOOP, packet received [1]", ret_code == COMMLAYER_RET_OK );
		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			//waitForTimeQuantum();
//			justWaitMSec( 200 );

//			return 0;
wait_for_comm_event:
//			ret_code = wait_for_communication_event( MEMORY_HANDLE_MAIN_LOOP, 1000 ); // TODO: recalculation
//			ret_code = wait_for_communication_event( 1000 ); // TODO: recalculation
//#ifdef TEST_AVR
#if !defined TEST_RAM_CONSUMPTION
			ret_code = hal_wait_for( &wait_for );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

//			ZEPTO_DEBUG_PRINTF_4( "=============================================Msg wait event; ret = %d, rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

			switch ( ret_code )
			{
				case WAIT_RESULTED_IN_FAILURE:
				{
					// regular processing will be done below in the next block
					return 0;
					break;
				}
				case WAIT_RESULTED_IN_PACKET:
				{
					// regular processing will be done below in the next block
					ret_code = try_get_message( MEMORY_HANDLE_MAIN_LOOP );
					if ( ret_code == COMMLAYER_RET_FAILED )
						return 0;
					ZEPTO_DEBUG_ASSERT( ret_code == COMMLAYER_RET_OK );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					goto saoudp_rec;
					break;
				}
				case WAIT_RESULTED_IN_TIMEOUT:
				{
//					if ( sagdp_data.event_type ) //TODO: temporary solution
					{
//						ZEPTO_DEBUG_PRINTF_1( "no reply received; the last message (if any) will be resent by timer\n" );
						sa_get_time( &(currt) );
						ret_code = handler_sagdp_timer( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
						if ( ret_code == SAGDP_RET_OK )
						{
							zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
							goto wait_for_comm_event;
						}
						else if ( ret_code == SAGDP_RET_NEED_NONCE )
						{
							ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
							ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
							sa_get_time( &(currt) );
							ret_code = handler_sagdp_timer( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
							ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_OK );
							zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
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
#else // TEST_RAM_CONSUMPTION
			for ( j=0; j<sizeof(INCOMING_TEST_PACKET); j++ )
				zepto_write_uint8( MEMORY_HANDLE_MAIN_LOOP, ZEPTO_PROG_CONSTANT_READ_BYTE( INCOMING_TEST_PACKET + j ) );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			goto sasp_rec;
#endif // TEST_RAM_CONSUMPTION


//			if ( timer_val && getTime() >= wake_time )
//			if ( tact.action )
//			if ( sagdp_data.event_type ) //TODO: temporary solution
			if ( 1 ) //TODO: temporary solution
			{
				ZEPTO_DEBUG_PRINTF_1( "no reply received; the last message (if any) will be resent by timer\n" );
				sa_get_time( &(currt) );
				ret_code = handler_sagdp_timer( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
				if ( ret_code == SAGDP_RET_OK )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					goto wait_for_comm_event;
				}
				else if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
					ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
					sa_get_time( &(currt) );
					ret_code = handler_sagdp_timer( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
		ZEPTO_DEBUG_PRINTF_2( "ret_code = %d\n", ret_code );
					ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_OK );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					goto saspsend;
					break;
				}
				else
				{
					ZEPTO_DEBUG_PRINTF_2( "ret_code = %d\n", ret_code );
					ZEPTO_DEBUG_ASSERT( 0 );
				}
			}
			else if ( wait_for_incoming_chain_with_timer && getTime() >= wake_time_to_start_new_chain )
			{
				wait_for_incoming_chain_with_timer = false;
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto alt_entry;
				break;
			}
/*trystart_over:
			ret_code = try_get_message( MEMORY_HANDLE_MAIN_LOOP );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER_IF( 92, "MAIN LOOP, packet received [2]", ret_code == COMMLAYER_RET_OK );
			ZEPTO_DEBUG_ASSERT( ret_code != COMMLAYER_RET_OK || ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ) != 0 );*/
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			ZEPTO_DEBUG_PRINTF_1("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communication_initialize()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto start_over;
		}
		ZEPTO_DEBUG_PRINTF_1("Message from client received\n");
		ZEPTO_DEBUG_PRINTF_4( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );


		// 2.1. Pass to SAoUDP
saoudp_rec:
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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}


		// 2.2. Pass to SASP
#ifdef TEST_RAM_CONSUMPTION
	sasp_rec:
#endif
#ifdef ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SASP_INCOMING_MESSAGE (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		ret_code = handler_sasp_receive( AES_ENCRYPTION_KEY, pid, MEMORY_HANDLE_MAIN_LOOP/*, &sasp_data*/ );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_PRINTF_4( "SASP1:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );
		switch ( ret_code )
		{
			case SASP_RET_IGNORE:
			{
				ZEPTO_DEBUG_PRINTF_1( "BAD MESSAGE_RECEIVED\n" );
				goto start_over;
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
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED: // as a result of error-old-nonce
			{
				ZEPTO_DEBUG_PRINTF_1( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				sa_get_time( &(currt) );
				ret_code = handler_sagdp_receive_request_resend_lsp( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
					ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
					sa_get_time( &(currt) );
					ret_code = handler_sagdp_receive_request_resend_lsp( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
					ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
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
		sa_get_time( &(currt) );
#ifdef ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SAGDP_INCOMING_MESSAGE (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		ret_code = handler_sagdp_receive_up( &currt, &wait_for, NULL, pid, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
		if ( ret_code == SAGDP_RET_START_OVER_FIRST_RECEIVED )
		{
//			sagdp_init( &sagdp_data );
			sagdp_init();
			// TODO: do remaining reinitialization
			sa_get_time( &(currt) );
			ret_code = handler_sagdp_receive_up( &currt, &wait_for, NULL, pid, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_START_OVER_FIRST_RECEIVED );
		}
		else if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
			sa_get_time( &(currt) );
			ret_code = handler_sagdp_receive_up( &currt, &wait_for, nonce, pid, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_PRINTF_4( "SAGDP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinitialize all
//				sagdp_init( &sagdp_data );
				sagdp_init();
//				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
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
				goto start_over;
			}
			default:
			{
				// unexpected ret_code
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}


		// 4. Process received command (yoctovm)
#ifdef ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SACCP_INCOMING_MESSAGE (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP, /*sasp_nonce_type chain_id*/NULL, &ret_wf ); // slave_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP );
/*		if ( ret_code == YOCTOVM_RESET_STACK )
		{
//			sagdp_init( &sagdp_data );
			sagdp_init();
			ZEPTO_DEBUG_PRINTF_1( "slave_process(): ret_code = YOCTOVM_RESET_STACK\n" );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
		}*/
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_PRINTF_4( "SACCP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		wait_for_incoming_chain_with_timer = false;

		switch ( ret_code )
		{
			case SACCP_RET_PASS_LOWER:
			{
				 // test generation: sometimes slave can start a new chain at not in-chain reason (although in this case it won't be accepted by Master)
//				bool restart_chain = tester_get_rand_val() % 8 == 0;
				bool restart_chain = false;
				if ( restart_chain )
				{
//					sagdp_init( &sagdp_data );
					sagdp_init();
					ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP, /*sasp_nonce_type chain_id*/NULL, &ret_wf ); // master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4*/ )
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					ZEPTO_DEBUG_ASSERT( ret_code == SACCP_RET_PASS_LOWER );
				}
				// regular processing will be done below in the next block
				break;
			}
		}


		// 5. SAGDP
alt_entry:
//		uint8_t timer_val;
		sa_get_time( &(currt) );
#ifdef ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SAGDP_INCOMING_MESSAGE [back] (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		ret_code = handler_sagdp_receive_hlp( &currt, &wait_for, NULL, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE/*, &sasp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code == SASP_RET_NONCE );
			sa_get_time( &(currt) );
			ret_code = handler_sagdp_receive_hlp( &currt, &wait_for, nonce, MEMORY_HANDLE_MAIN_LOOP/*, &sagdp_data*/ );
			ZEPTO_DEBUG_ASSERT( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_PRINTF_4( "SAGDP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

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
				goto start_over;
				break;
			}
			case SAGDP_RET_TO_LOWER_REPEATED: // TODO: is it possible here?
			{
				goto start_over;
				break;
			}
			case SAGDP_RET_SYS_CORRUPTED: // TODO: is it possible here?
			{
				// TODO: process reset
				sagdp_init();
//				bool start_now = tester_get_rand_val() % 3;
//				bool start_now = true;
//				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wake_time_to_start_new_chain = getTime();
				wait_for_incoming_chain_with_timer = true;
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
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

		// SASP
saspsend:
#ifdef ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SASP_INCOMING_MESSAGE [back] (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_INCOMING_MESSAGE
		ret_code = handler_sasp_send( AES_ENCRYPTION_KEY, nonce, MEMORY_HANDLE_MAIN_LOOP/*, &sasp_data*/ );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		ZEPTO_DEBUG_PRINTF_4( "SASP2:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

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


		// Pass to SAoUDP
saoudp_send:
#ifdef ALLOW_PRINTING_SASP_OUTGOING_MESSAGE
		{
			parser_obj po;
			int ctr = 0;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			ZEPTO_DEBUG_PRINTF_2( "SASP_OUTGOING_MESSAGE (%d bytes):\n", zepto_parsing_remaining_bytes( &po ) );
			while ( zepto_parsing_remaining_bytes( &po ) != 0 )
			{
				ZEPTO_DEBUG_PRINTF_2( "0x%02x, ", zepto_parse_uint8( &po ) );
				ctr++;
				if ( (ctr & 7) == 0 )
					ZEPTO_DEBUG_PRINTF_1( "\n" );
			}
			ZEPTO_DEBUG_PRINTF_1( "\n\n" );
		}
#endif // ALLOW_PRINTING_SASP_OUTGOING_MESSAGE
#ifdef TEST_RAM_CONSUMPTION
		{
			parser_obj po;
			zepto_parser_init( &po, MEMORY_HANDLE_MAIN_LOOP );
			test_byte = true;
			for ( j=0; j<sizeof(OUTGOING_TEST_PACKET); j++ )
			{
				test_byte = test_byte && zepto_parsing_remaining_bytes( &po ) != 0 && ZEPTO_PROG_CONSTANT_READ_BYTE( OUTGOING_TEST_PACKET + j ) == zepto_parse_uint8( &po );
			}
			ZEPTO_DEBUG_PRINTF_2( "Testing output: %s\n\n", test_byte ? "OK" : "FAILED" );
			test_byte ++;
			return 0;
		}
#endif // TEST_RAM_CONSUMPTION
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
				ZEPTO_DEBUG_PRINTF_2( "Unexpected ret_code %d\n", ret_code );
				ZEPTO_DEBUG_ASSERT( 0 );
				break;
			}
		}

			ret_code = send_message( MEMORY_HANDLE_MAIN_LOOP );
			if (ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER( 90, "MAIN LOOP, packet sent" );
			ZEPTO_DEBUG_PRINTF_1("\nMessage replied to client\n");

start_over:;
#ifdef TEST_RAM_CONSUMPTION
			return 0;
#endif // TEST_RAM_CONSUMPTION
	}

	return 0;
}
