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



int main(int argc, char *argv[])
{
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	initTestSystem();
	bool run_simultaniously = true;
	if ( run_simultaniously )
	{
		requestSyncExec();
	}

	// in this preliminary implementation all memory segments are kept separately
	// All memory objects are listed below
	// TODO: revise memory management
	uint16_t* sizeInOut = (uint16_t*)(rwBuff + 3 * BUF_SIZE / 4);
	uint8_t* stack = (uint8_t*)sizeInOut + 2; // first two bytes are used for sizeInOut
	uint16_t stackSize = BUF_SIZE / 4 - 2;
	uint8_t nonce[ SASP_NONCE_SIZE ];
	uint8_t timer_val = 0;
	uint16_t wake_time;
	// TODO: revise time/timer management

	uint8_t ret_code;

	// test setup values
	bool wait_for_incoming_chain_with_timer;
	uint16_t wake_time_to_start_new_chain;

	// do necessary initialization
	sagdp_init( data_buff + DADA_OFFSET_SAGDP );

	// Try to open a named pipe; wait for it, if necessary. 
	if ( !communicationInitializeAsClient() )
		return -1;




	ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
	memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
	goto entry;

	// MAIN LOOP
	for (;;)
	{
getmsg:
		// 1. Get message from comm peer
		printf("Waiting for a packet from server...\n");
//		ret_code = getMessage( sizeInOut, rwBuff, BUF_SIZE);
		ret_code = tryGetMessage( sizeInOut, rwBuff, BUF_SIZE);

		// sync sending/receiving
		if ( ret_code == COMMLAYER_RET_OK )
		{
			uint16_t sz;
			if ( releaseOutgoingPacket( rwBuff + BUF_SIZE / 4, &sz ) ) // we are on the safe side here as buffer out is not yet used
				sendMessage( &sz, rwBuff + BUF_SIZE / 4 );
		}

		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			waitForTimeQuantum();
			if ( timer_val != 0 && getTime() >= wake_time )
			{
				printf( "no reply received; the last message (if any) will be resent by timer\n" );
				// TODO: to think: why do we use here handlerSAGDP_receiveRequestResendLSP() and not handlerSAGDP_timer()
				ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, NULL, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handlerSASP_get_nonce( sizeInOut, nonce, SASP_NONCE_SIZE, stack, stackSize, data_buff + DADA_OFFSET_SASP );
					assert( ret_code == SASP_RET_NONCE );
					ret_code = handlerSAGDP_receiveRequestResendLSP( &timer_val, nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
					assert( ret_code != SAGDP_RET_NEED_NONCE );
				}
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
				wake_time = 0;
				// sync sending/receiving: release presently hold packet (if any), and cause this packet to be hold
				{
					uint16_t sz;
					if ( releaseOutgoingPacket( rwBuff + BUF_SIZE / 4, &sz ) ) // we are on the safe side here as buffer out is not yet used
					{
						sendMessage( &sz, rwBuff + BUF_SIZE / 4 );
						requestHoldingPacket();
					}
				}

				goto saspsend;
				break;
			}
			else if ( wait_for_incoming_chain_with_timer && getTime() >= wake_time )
			{
				wait_for_incoming_chain_with_timer = false;
				ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
				goto entry;
				break;
			}
			ret_code = tryGetMessage( sizeInOut, rwBuff, BUF_SIZE);

			// sync sending/receiving
			if ( ret_code == COMMLAYER_RET_OK )
			{
				uint16_t sz;
				if ( releaseOutgoingPacket( rwBuff + BUF_SIZE / 4, &sz ) ) // we are on the safe side here as buffer out is not yet used
					sendMessage( &sz, rwBuff + BUF_SIZE / 4 );
			}
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communicationInitializeAsClient()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}
		printf("Message from server received\n");
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
		if ( ret_code == SAGDP_RET_NEED_NONCE )
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
				ret_code = master_error( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
				memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
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
		ret_code = master_continue( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
		if ( ret_code == YOCTOVM_RESET_STACK )
		{
			sagdp_init( data_buff + DADA_OFFSET_SAGDP );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
		}
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "YOCTO:  ret: %d; size: %d\n", ret_code, *sizeInOut );
entry:	

		switch ( ret_code )
		{
			case YOCTOVM_PASS_LOWER:
			{
				 // test generation: sometimes master can start a new chain at not in-chain reason
				bool restart_chain = get_rand_val() % 8 == 0;
				if ( restart_chain )
				{
					sagdp_init( data_buff + DADA_OFFSET_SAGDP );
					ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
					memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
					assert( ret_code == YOCTOVM_PASS_LOWER );
				}
				// regular processing will be done below in the next block
				break;
			}
			case YOCTOVM_PASS_LOWER_THEN_IDLE:
			{
				bool start_now = get_rand_val() % 3 == 0;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + get_rand_val() % 8;
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
				bool start_now = get_rand_val() % 3 == 0;
				if ( start_now )
				{
					ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4/*, BUF_SIZE / 4, stack, stackSize*/ );
					memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
					assert( ret_code == YOCTOVM_PASS_LOWER );
					// one more trick: wait for some time to ensure that slave will start its own chain, and then send "our own" chain start
					bool mutual = get_rand_val() % 5 == 0;
					if ( mutual )
						justWait( 5 );
				}
				else
				{
					wake_time_to_start_new_chain = getTime() + get_rand_val() % 8;
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
		ret_code = handlerSASP_send( nonce, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
		printf( "SASP2: ret: %d; size: %d\n", ret_code, *sizeInOut );

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
		allowSyncExec();
		registerOutgoingPacket( rwBuff, *sizeInOut );

		bool syncSendReceive;
		if ( holdPacketOnRequest( rwBuff, sizeInOut ) )
			syncSendReceive = false;
		else
			get_rand_val() % 7 == 0 && !isOutgoingPacketOnHold();

		if ( syncSendReceive )
		{
			holdOutgoingPacket( rwBuff, sizeInOut );
		}
		else
		{
			if ( shouldInsertOutgoingPacket( rwBuff, sizeInOut ) )
				sendMessage( sizeInOut, rwBuff );

			if ( !shouldDropOutgoingPacket() )
			{
				ret_code = sendMessage( sizeInOut, rwBuff );
				if (ret_code != COMMLAYER_RET_OK )
				{
					return -1;
				}
				printf("\nMessage sent to comm peer\n");
			}
			else
			{
				printf("\nMessage lost on the way...\n");
			}
		}


		// for testing purposes
/*		if ( !isChainContinued() )
		{
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
			memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );
			goto entry;
		}*/
	}

	communicationTerminate();

	return 0;
}

