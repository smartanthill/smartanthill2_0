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


int dummy_message_count; // used for fake message generation

int prepareInitialMessage( unsigned char* buff, int buffSize )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	memset( buff, 'Z', buffSize );
	sprintf( (char*)buff, "Client message #%d", dummy_message_count++ );
	int size = 0;
	while ( buff[size++] );
	printf("Preparing %d byte message: \"%s\"\n", size, buff);
	return size;
}

void postprocessReceivedMessage( unsigned char* buff, int msgSize, int buffSize )
{
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	printf("Message received: \"%s\" [%d bytes]\n", buff, msgSize);
}


int main(int argc, char *argv[])
{
	printf("starting CLIENT...\n");
	printf("==================\n\n");

	bool   fSuccess = false;
	int msgSize;

	// Try to open a named pipe; wait for it, if necessary. 

	fSuccess = communicationInitializeAsClient();
	if (!fSuccess)
		return -1;

	do
	{
		msgSize = prepareInitialMessage( rwBuff, BUF_SIZE/2 );

		unsigned char msgCopy[BUF_SIZE], msgBack[BUF_SIZE];
		memcpy(msgCopy, rwBuff, msgSize);
		int msgSizeCopy = msgSize;
		int msgSizeBack;

		msgSize = handlerSASP_send( 0, rwBuff, msgSize, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4 );
		memcpy( rwBuff, rwBuff + BUF_SIZE / 4, msgSize );

		msgSizeBack = handlerSASP_receive( rwBuff, msgSize, msgBack, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4 );
		assert( msgSizeCopy+1 == msgSizeBack );
		for ( int k=0; k<msgSizeCopy; k++ )
			assert( msgCopy[k] == msgBack[k+1] );

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

		printf("\n... Message received from server [%d bytes]:\n", msgSize);


		// process received
		msgSize = handlerSASP_receive( rwBuff, msgSize, rwBuff + BUF_SIZE / 4, BUF_SIZE / 4, rwBuff + 3 * BUF_SIZE / 4, BUF_SIZE / 4 );

		postprocessReceivedMessage( rwBuff + BUF_SIZE / 4 + 1, msgSize, BUF_SIZE );

		// repeating?..
		printf("\n<End of message, press X+ENTER to terminate connection and exit or ENTER to continue>");
		char c = getchar();
		if (c == 'x' || c == 'X')
			break;
		printf( "\n\n" );
	} 
	while (1);

	communicationTerminate();

	return 0;
}

