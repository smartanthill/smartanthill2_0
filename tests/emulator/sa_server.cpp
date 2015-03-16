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
uint8_t rwBuff[BUF_SIZE];
uint8_t data_buff[BUF_SIZE];
uint8_t msgLastSent[BUF_SIZE];
uint8_t pid[ SASP_NONCE_SIZE ];
uint8_t nonce[ SASP_NONCE_SIZE ];




int main(int argc, char *argv[])
{
	printf("STARTING SERVER...\n");
	printf("==================\n\n");

	initTestSystem();


	// in this preliminary implementation all memory segments are kept separately
	// All memory objects are listed below
	// TODO: revise memory management
	uint16_t* sizeInOut = (uint16_t*)(rwBuff + 3 * BUF_SIZE / 4);
	uint8_t* stack = (uint8_t*)sizeInOut + 2; // first two bytes are used for sizeInOut
	int stackSize = BUF_SIZE / 4 - 2;
	uint8_t timer_val;
	uint16_t wake_time;
	// TODO: revise time/timer management


	uint8_t ret_code;

	// do necessary initialization
	sagdp_init( data_buff + DADA_OFFSET_SAGDP );


	printf("\nAwaiting client connection... \n" );
	if (!communicationInitializeAsServer())
		return -1;

	printf("Client connected.\n");

	bool startSeq = startSequence();

	// MAIN LOOP
	for (;;)
	{
		if ( startSeq )
		{
			startSeq = false;
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
			waitToProceed();
			memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
			goto alt_entry;
		}
getmsg:
		// 1. Get message from comm peer
/*		ret_code = getMessage( sizeInOut, rwBuff, BUF_SIZE);
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR A NEW CLIENT...\n\n");
			if (!communicationInitializeAsServer()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}*/
		ret_code = tryGetMessage( sizeInOut, rwBuff, BUF_SIZE);
		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			waitForTimeQuantum();
			if ( timer_val && getTime() >= wake_time )
			{
				printf( "no reply received; the last message (if any) will be resent by timer\n" );
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handlerSASP_get_nonce( sizeInOut, nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					assert( ret_code != SAGDP_RET_NEED_NONCE );
				}
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
				goto saspsend;
				break;
			}
			ret_code = tryGetMessage( sizeInOut, rwBuff, BUF_SIZE);
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communicationInitializeAsClient()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}
		printf("Message from client received\n");
		printf( "ret: %d; size: %d\n", ret_code, *sizeInOut );

rectosasp:
		// 2. Pass to SASP
		ret_code = handlerSASP_receive( pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "SASP1:  ret: %d; size: %d\n", ret_code, *sizeInOut );

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
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
				goto saspsend;
				break;
			}*/
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED:
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handlerSASP_get_nonce( sizeInOut, nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					assert( ret_code != SAGDP_RET_NEED_NONCE );
				}
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
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
		ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		if ( ret_code == SAGDP_RET_START_OVER_FIRST_RECEIVED )
		{
			sagdp_init( data_buff + DADA_OFFSET_SAGDP );
			// TODO: do remaining reinitialization
			ret_code = handlerSAGDP_receiveUP( &timer_val, NULL, pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			assert( ret_code != SAGDP_RET_START_OVER_FIRST_RECEIVED );
		}
		else if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handlerSASP_get_nonce( sizeInOut, nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
			assert( ret_code == SASP_RET_NONCE );
			ret_code = handlerSAGDP_receiveUP( &timer_val, nonce, pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "SAGDP1: ret: %d; size: %d\n", ret_code, *sizeInOut );

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
				goto getmsg;
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
		ret_code = slave_process( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "YOCTO:  ret: %d; size: %d\n", ret_code, *sizeInOut );

		switch ( ret_code )
		{
			case YOCTOVM_PASS_LOWER:
			{
				// regular processing will be done below in the next block
				break;
			}
			case YOCTOVM_OK:
			{
				goto getmsg;
				break;
			}
			case YOCTOVM_FAILED:
			{
				// TODO: process reset
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
alt_entry:
		uint8_t timer_val;
		ret_code = handlerSAGDP_receiveHLP( &timer_val, NULL, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handlerSASP_get_nonce( sizeInOut, nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
			assert( ret_code == SASP_RET_NONCE );
			ret_code = handlerSAGDP_receiveHLP( &timer_val, nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "SAGDP2: ret: %d; size: %d\n", ret_code, *sizeInOut );

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
				// TODO: process reset
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

		// SASP
saspsend:
		ret_code = handlerSASP_send( nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "SASP2:  ret: %d; size: %d\n", ret_code, *sizeInOut );

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

/*		ret_code = handlerSAGDP_receivePID( pid, data_buff + DADA_OFFSET_SAGDP );
		printf( "SAGDP3: ret: %d; size: %d\n", ret_code, *sizeInOut );
		switch ( ret_code )
		{
			case SAGDP_RET_OK:
			{
				// regular processing will be done below in the next block
				break;
			}
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: process reset
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
		*/
sendmsg:
			ret_code = sendMessage( sizeInOut, rwBuff );
			if (ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			printf("\nMessage replied to client\n");

	}

	return 0;
}
