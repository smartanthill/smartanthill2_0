/*******************************************************************************
    Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source and compiled
    forms, with or without modification, are permitted
    provided that the following conditions are met:
        * Redistributions in source form must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in compiled form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the OLogN Technologies AG nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE
*******************************************************************************/


#include "sa-common.h"
#include "sa-commlayer.h"
#include "sa-timer.h"
#include "sasp_protocol.h"
#include "sagdp_protocol.h"
#include "yoctovm_protocol.h"
#include <stdio.h> 


#define BUF_SIZE 512
unsigned char rwBuff[BUF_SIZE];
unsigned char data_buff[BUF_SIZE];
unsigned char msgLastSent[BUF_SIZE];
uint8_t pid[ SASP_NONCE_SIZE ];

/*
int dummy_message_count; // used for fake message generation

uint8_t prepareInitialMessage( uint16_t* sizeInOut, unsigned char* buff, uint16_t max_size )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	uint16_t buffSize = *sizeInOut;
	buff[0] = 0; // First Byte
	buff++;
	memset( buff, 'Z', max_size-1 );
	sprintf( (char*)buff, "Client message #%d", dummy_message_count++ );
	uint16_t size = 0;
	while ( buff[size++] );
	printf("Preparing 1+%d byte message: \"%s\"\n", size, buff);
	*sizeInOut = size + 1;
	return 1;
}

uint8_t postprocessReceivedMessage( uint16_t* sizeInOut, unsigned char* buff )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	printf("Message received: \"%s\" [%d bytes]\n", buff, *sizeInOut);
	return 1;
}
*/


int main(int argc, char *argv[])
{
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	// TODO: revise approach below
	uint16_t* sizeInOut = (uint16_t*)(rwBuff + 3 * BUF_SIZE / 4);
	uint8_t* stack = (uint8_t*)sizeInOut + 2; // first two bytes are used for sizeInOut
	int stackSize = BUF_SIZE / 4 - 2;


	// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
/*	unsigned char msgLastSent[BUF_SIZE];
	uint16_t sizeInOutLastSent;
	sizeInOutLastSent = 0;
	bool resendLastMsg = false;*/

	uint8_t ret_code;



	// Try to open a named pipe; wait for it, if necessary. 
	if ( !communicationInitializeAsClient() )
		return -1;

	ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
	goto entry;

	// MAIN LOOP
	for (;;)
	{
getmsg:
		// 1. Get message from comm peer
		ret_code = getMessage( sizeInOut, rwBuff, BUF_SIZE);
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR A NEW CLIENT...\n\n");
			if (!communicationInitializeAsServer()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}
		printf("Message from client received\n");

rectosasp:
		// 2. Pass to SASP
		ret_code = handlerSASP_receive( pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

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
			case SASP_RET_TO_HIGHER_REPEATED:
			{
				// goto ...
				break;
			}
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED:
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				// goto ...
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
		ret_code = handlerSAGDP_receiveNewUP( pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

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
entry:	memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

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
		uint8_t timer_val;
		ret_code = handlerSAGDP_receiveHLP( &timer_val, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SAGDP, msgLastSent );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

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
		ret_code = handlerSASP_send( false, pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

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

		ret_code = handlerSAGDP_receivePID( pid, data_buff + DADA_OFFSET_SAGDP );
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

sendmsg:
			ret_code = sendMessage( sizeInOut, rwBuff );
			if (ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			printf("\nMessage replied to client\n");

	}
#if 0
	do
	{
		if ( resendLastMsg )
		{
			// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
			memcpy( rwBuff, msgLastSent, *sizeInOut );
			*sizeInOut = sizeInOutLastSent;
		}
		else
		{
			prepareInitialMessage( sizeInOut, rwBuff, BUF_SIZE/2 );
			// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
			memcpy( msgLastSent, rwBuff, *sizeInOut );
			sizeInOutLastSent = *sizeInOut;
		}
		resendLastMsg = false;

		uint8_t ret_code = handlerSASP_send( false, pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		assert( ret_code == SASP_RET_TO_LOWER ); // is anything else possible?
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, *sizeInOut );

		// send ... receive ...
		printf( "raw sent: \"%s\"\n", rwBuff );
		ret_code = sendMessage( sizeInOut, rwBuff );

		if (!fSuccess)
		{
			return -1;
		}

		printf("\nMessage sent to server [%d bytes]...\n", *sizeInOut );

		ret_code = getMessage(sizeInOut, rwBuff, BUF_SIZE);

		if ( ret_code != COMMLAYER_RET_OK )
		{
			return -1;
		}

		printf("\n... Message received from server [%d bytes]:\n", *sizeInOut );


		// process received
		ret_code = handlerSASP_receive( pid, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		if ( ret_code == SASP_RET_IGNORE )
		{
			printf( "BAD MESSAGE_RECEIVED\n" );
		}
		else if ( ret_code == SASP_RET_TO_HIGHER_NEW || ret_code == SASP_RET_TO_HIGHER_REPEATED )
		{
			postprocessReceivedMessage( sizeInOut, rwBuff + BUF_SIZE / 4 + 1 );
			// repeating?..
			printf("\n<End of message, press X+ENTER to terminate connection and exit or ENTER to continue>");
			char c = getchar();
			if (c == 'x' || c == 'X')
				break;
			printf( "\n\n" );
		}
		else if ( ret_code == SASP_RET_TO_LOWER )
		{
			printf( "OLD NONCE detected\n" );
			ret_code = sendMessage( sizeInOut, rwBuff );
			if ( ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			printf("\nError Message replied to server\n");
		}
		else if (ret_code == SASP_RET_TO_HIGHER_LAST_SEND_FAILED)
		{
			printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
			resendLastMsg = true;
		}
		else
		{
			assert(0);
		}


	} 
	while (1);
#endif // 0

	communicationTerminate();

	return 0;
}

