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
#include <stdio.h> 


#define BUF_SIZE 512
unsigned char rwBuff[BUF_SIZE];
unsigned char data_buff[BUF_SIZE];


int dummy_message_count; // used for fake message generation

int prepareInitialMessage( unsigned char* buff, int buffSize )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	buff[0] = 0; // First Byte
	buff++;
	memset( buff, 'Z', buffSize-1 );
	sprintf( (char*)buff, "Client message #%d", dummy_message_count++ );
	int size = 0;
	while ( buff[size++] );
	printf("Preparing 1+%d byte message: \"%s\"\n", size, buff);
	return size+1;
}

void postprocessReceivedMessage( unsigned char* buff, int msgSize, int buffSize )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	printf("Message received: \"%s\" [%d bytes]\n", buff, msgSize);
}



// Temporary solutuion to work with sizes
// TODO: implement an actual mechanism
int sizeInOutToInt( const uint8_t* sizeInOut )		{int size = sizeInOut[1];size <<= 8;size = sizeInOut[0];return size;}
void loadSizeInOut( uint8_t* sizeInOut, int size )	{sizeInOut[1] = size >> 8; sizeInOut[0] = size & 0xFF;}


int main(int argc, char *argv[])
{
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	// TODO: revise approach below
	uint8_t* sizeInOut = rwBuff + 3 * BUF_SIZE / 4;
	uint8_t* stack = sizeInOut + 2; // first two bytes are used for sizeInOut
	int stackSize = BUF_SIZE / 4 - 2;

	// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
	unsigned char msgLastSent[BUF_SIZE], sizeInOutLastSent[2];
	loadSizeInOut( sizeInOutLastSent, 0 );
	bool resendLastMsg = false;

	// debug objects
	unsigned char msgCopy[BUF_SIZE], msgBack[BUF_SIZE], sizeInOutCopy[2], sizeInOutBack[2];
	int msgSizeCopy, msgSizeBack;



	bool   fSuccess = false;
	int msgSize;

	// Try to open a named pipe; wait for it, if necessary. 

	fSuccess = communicationInitializeAsClient();
	if (!fSuccess)
		return -1;

	do
	{
		if ( resendLastMsg )
		{
			// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
			memcpy( rwBuff, msgLastSent, msgSize );
			memcpy( sizeInOut, sizeInOutLastSent, 2 );
			msgSize = sizeInOutToInt( sizeInOut );
		}
		else
		{
			msgSize = prepareInitialMessage( rwBuff, BUF_SIZE/2 );
			loadSizeInOut( sizeInOut, msgSize );
			// quick simulation of a part of SAGDP responsibilities: a copy of the last message sent message
			memcpy( msgLastSent, rwBuff, msgSize );
			memcpy( sizeInOutLastSent, sizeInOut, 2 );
		}
		resendLastMsg = false;

		// check block #1: copy the message
		memcpy(msgCopy, rwBuff, msgSize);
		loadSizeInOut( sizeInOutCopy, msgSize );

		uint8_t ret_code = handlerSASP_send( false, sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		assert( ret_code == SASP_RET_TO_LOWER ); // is anything else possible?
		msgSize = sizeInOutToInt( sizeInOut );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, msgSize );

		// check block #2: ensure SASP block is done well
		memcpy( sizeInOutBack, sizeInOut, 2 );
		bool checkOK = SASP_IntraPacketAuthenticateAndDecrypt( sizeInOutBack, rwBuff, msgBack, BUF_SIZE / 4, stack, stackSize );
		assert( checkOK );
		assert( sizeInOutCopy[0] == sizeInOutBack[0] && sizeInOutCopy[1] == sizeInOutBack[1] );
		msgSizeCopy = sizeInOutToInt( sizeInOutCopy );
		msgBack[0] &= 0x7F; // get rid of SASP bit
		for ( int k=0; k<msgSizeCopy; k++ )
			assert( msgCopy[k] == msgBack[k] );

		// send ... receive ...
		printf( "raw sent: \"%s\"\n", rwBuff );
		fSuccess = sendMessage( rwBuff, msgSize );

		if (!fSuccess)
		{
			return -1;
		}

		printf("\nMessage sent to server [%d bytes]...\n", msgSize );

		msgSize = getMessage(rwBuff, BUF_SIZE);

		if (!fSuccess)
		{
			return -1;
		}
		loadSizeInOut( sizeInOut, msgSize );

		printf("\n... Message received from server [%d bytes]:\n", msgSize);


		// process received
		ret_code = handlerSASP_receive( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, stack, stackSize, data_buff + DADA_OFFSET_SASP );
		if ( ret_code == SASP_RET_IGNORE )
		{
			printf( "BAD MESSAGE_RECEIVED\n" );
		}
		else if ( ret_code == SASP_RET_TO_HIGHER_NEW || ret_code == SASP_RET_TO_HIGHER_REPEATED )
		{
			postprocessReceivedMessage( rwBuff + BUF_SIZE / 4 + 1, msgSize, BUF_SIZE );
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
			bool fSuccess = sendMessage((unsigned char *)rwBuff, msgSize);
			if (!fSuccess)
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

	communicationTerminate();

	return 0;
}

