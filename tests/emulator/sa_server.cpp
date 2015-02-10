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


#include "sa-commlayer.h"
#include <stdio.h> 

#define BUF_SIZE 512

int main(int argc, char *argv[])
{
	printf("starting server...\n");

	bool   fConnected = false;


	printf("\nPipe Server: Main thread awaiting client connection on %s\n", g_pipeName);
	if (!communicationInitializeAsServer())
		return -1;

	printf("Client connected.\n");

	unsigned char rwBuff[BUF_SIZE];

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

		// do some nonsence to imitate processing
		unsigned char temp;
		for (int i = 0; i < msgSize / 2 - 1; i++)
		{
			temp = rwBuff[i];
			rwBuff[i] = rwBuff[msgSize - 2 - i];
			rwBuff[msgSize - i] = temp;
		}
		rwBuff[msgSize - 1] = 0;

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
