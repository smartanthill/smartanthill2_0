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
#include "sasp_protocol.h"
#include <stdio.h> 


#define BUF_SIZE 512
unsigned char rwBuff[BUF_SIZE];
unsigned char data_buff[BUF_SIZE];




int prepareReplyMessage( unsigned char* buffIn, int msgSize, unsigned char* buffOut, int buffOutSize, unsigned char* stack, int stackSize )
{
	// buffer assumes to contain an input message; interface is subject to change
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	unsigned char flags = buffIn[0];
	unsigned char* payload_buff = buffIn + 1;
	printf("Preparing reply to client message: [0x%02x]\"%s\" [1+%d bytes]\n", flags, payload_buff, msgSize-1 );
	sprintf( (char*)buffOut + 1, "Server reply; client message: [0x%02x]\"%s\" [1+%d bytes]", flags, payload_buff, msgSize-1 );
	buffOut[0] = buffIn[0]; // just echo so far
	int size = 0;
	while ( (buffOut + 1)[size++] );
	printf("Reply is about to be sent: \"%s\" [%d bytes]\n", buffOut + 1, size);
	return size+1;
}

int main(int argc, char *argv[])
{
	printf("STARTING SERVER...\n");
	printf("==================\n\n");

	bool   fConnected = false;


	printf("\nPipe Server: Main thread awaiting client connection on %s\n", g_pipeName);
	if (!communicationInitializeAsServer())
		return -1;

	printf("Client connected.\n");


	for (;;)
	{
		int msgSize = getMessage(rwBuff, BUF_SIZE);
		if (msgSize == -1)
		{
			printf("\n\nWAITING FOR A NEW CLIENT...\n\n");
			if (!communicationInitializeAsServer()) // regardles of errors... quick and dirty solution so far
				return -1;
			continue;
		}

		printf("Message from client received:\n");
		printf("\"%s\"\n", rwBuff);

		// process received message
		msgSize = handlerSASP_receive( rwBuff, msgSize, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4, data_buff + DADA_OFFSET_SASP );
		if ( msgSize == -1 )
		{
			printf( "BAD MESSAGE_RECEIVED\n" );
		}
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, msgSize );
		msgSize = prepareReplyMessage( rwBuff, msgSize, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4 );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, msgSize );

		// check block #1
		unsigned char msgCopy[BUF_SIZE], msgBack[BUF_SIZE];
		memcpy( msgCopy, rwBuff, msgSize );
		int msgSizeCopy = msgSize, msgSizeBack;

		msgSize = handlerSASP_send( false, rwBuff[0], rwBuff+1, msgSize-1, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4, data_buff + DADA_OFFSET_SASP );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, msgSize );

		// check block #2
		msgSizeBack = SASP_IntraPacketAuthenticateAndDecrypt( rwBuff, msgSize, msgBack + BUF_SIZE / 4, BUF_SIZE / 4, msgBack + 3 * BUF_SIZE / 4, BUF_SIZE / 4 );
		memcpy( msgBack, msgBack + BUF_SIZE / 4, msgSizeBack );
		assert( msgSizeCopy == msgSizeBack );
		for ( int k=0; k<msgSizeCopy; k++ )
			assert( msgCopy[k] == msgBack[k] );

		// reply
		bool fSuccess = sendMessage((unsigned char *)rwBuff, msgSize);
		if (!fSuccess)
		{
			return -1;
		}
		printf("\nMessage replied to client, reply is:\n");
		printf("\"%s\"\n", rwBuff);

	}

	return 0;
}
