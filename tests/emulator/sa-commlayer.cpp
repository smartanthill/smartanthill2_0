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
#include <stdio.h> 


#define BUFSIZE 512

#ifdef _MSC_VER

#include <Windows.h>

HANDLE hPipe;
char* g_pipeName = "\\\\.\\pipe\\mynamedpipe";

OVERLAPPED readOverl;

bool communicationInitializeAsServer()
{
	hPipe = CreateNamedPipeA(
		g_pipeName,             // pipe name 
		PIPE_ACCESS_DUPLEX,       // read/write access 
//		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access 
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		BUFSIZE,                  // output buffer size 
		BUFSIZE,                  // input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
		return false;
	}

	// Wait for the client to connect; if it succeeds, 
	// the function returns a nonzero value. If the function
	// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

	bool connected = ConnectNamedPipe(hPipe, NULL) ?
	TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

	if (!connected)
		CloseHandle(hPipe);

	return connected;
}

bool communicationInitializeAsClient()
{
	memset(&readOverl, 0, sizeof(readOverl));
	readOverl.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	bool waitingForCreation = false;
	while (1)
	{
		hPipe = CreateFileA(
			g_pipeName,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
//			0,              // default attributes 
			FILE_FLAG_OVERLAPPED,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			waitingForCreation = true;
			printf(".");
			Sleep(1000);
			continue;
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			printf("Could not open pipe. GLE=%d\n", GetLastError());
			return false;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipeA(g_pipeName, 20000))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return false;
		}
	}
	if (waitingForCreation)
		printf("\n\n");

	return true;
}

void communicationTerminate()
{
	CloseHandle(hPipe);
	hPipe = INVALID_HANDLE_VALUE;
}

uint8_t sendMessage(uint16_t* msgSize, const unsigned char * buff)
{
	BOOL   fSuccess = FALSE;
	DWORD  cbWritten;
	unsigned char sizePacked[2];
	sizePacked[0] = *msgSize & 0xFF;
	sizePacked[1] = (*msgSize >> 8) & 0xFF;

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		sizePacked,             // message 
		2,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
		return COMMLAYER_RET_FAILED;
	}

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		buff,             // message 
		*msgSize,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
		return COMMLAYER_RET_FAILED;
	}

	return COMMLAYER_RET_OK;
}

uint8_t getMessage(uint16_t* msgSize, unsigned char * buff, int maxSize) // returns when a packet received
{
	BOOL   fSuccess = FALSE;
	DWORD  cbRead;
	int byteCnt = 0;
	*msgSize = -1;

	do
	{
		do
		{
			// Read from the pipe. 

			fSuccess = ReadFile(
				hPipe,    // pipe handle 
				buff + byteCnt,    // buffer to receive reply 
				1,  // size to read 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 

			if (!fSuccess)
				break;

			byteCnt++;

			if (byteCnt == 2)
			{
				*msgSize = buff[1];
				*msgSize <<= 8;
				*msgSize += buff[0];
				break;
			}
		} while (1);

		if (!fSuccess)
			break;

		byteCnt = 0;
		do
		{
			// Read from the pipe. 
			if (byteCnt >= *msgSize)
			{
				//			  printf( "\"%s\"\n", buff );
				break;
			}

			fSuccess = ReadFile(
				hPipe,    // pipe handle 
				buff + byteCnt,    // buffer to receive reply 
				1,  // size to read 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 

			if (!fSuccess)
				break;

			byteCnt++;

		} while (1);
		break;
	} while (1);

	return *msgSize == (uint16_t)(-1) ? COMMLAYER_RET_FAILED : COMMLAYER_RET_OK;
}


int asyncReadStatus = 0; // 0: nothing is started; 1: reading initiated; 2: size retrieved
uint8_t async_buff[BUFSIZE];

uint8_t initReading()
{
//	printf( "initReading() called...\n" );
	BOOL   fSuccess = FALSE;
	DWORD  cbRead;

	asyncReadStatus = 0;

	fSuccess = ReadFile(
		hPipe,    // pipe handle 
		async_buff,    // buffer to receive reply 
		2,  // size to read 
		&cbRead,  // number of bytes read 
		&readOverl);    // overlapped 

	if (!fSuccess || cbRead < 2)
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			asyncReadStatus = 1;
			return COMMLAYER_RET_PENDING;
		}
		else
			return COMMLAYER_RET_FAILED;
	}
	else
	{
		asyncReadStatus = 2;
		return COMMLAYER_RET_OK;
	}
}

bool isPreReadingDone()
{
			Sleep(1);
//	printf( "isPreReadingDone() called...\n" );
	assert( asyncReadStatus == 1 );
	DWORD retrieved = 0;
	BOOL ret = GetOverlappedResult( hPipe, &readOverl, &retrieved, FALSE );
	if ( ! ret )
	{
		DWORD error = GetLastError();
		if ( error == ERROR_IO_INCOMPLETE )
			return false;
		printf( "Unexpected error %d\n", error );
		assert(0);
	}
	assert( retrieved == 2 );
	asyncReadStatus = 2;
	return true;
}

uint8_t finalizeReading(uint16_t* msgSize, unsigned char * buff, int maxSize)
{
//	printf( "finalizeReading() called (size = %d)...\n", async_buff[0] );
	BOOL   fSuccess = FALSE;
	DWORD  cbRead;

	*msgSize = async_buff[1];
	*msgSize <<= 8;
	*msgSize += async_buff[0];

	fSuccess = ReadFile(
		hPipe,    // pipe handle 
		buff,    // buffer to receive reply 
		*msgSize,  // size to read 
		&cbRead,  // number of bytes read 
		NULL);    // not overlapped 

	asyncReadStatus = 0;
	if ( fSuccess )
	{
		return COMMLAYER_RET_OK;
	}
	else
	{
		return COMMLAYER_RET_FAILED;
	};

}

uint8_t tryGetMessage(uint16_t* msgSize, unsigned char * buff, int maxSize) // returns immediately, but a packet reception is not guaranteed
{
	uint8_t ret_code;
	if ( asyncReadStatus == 0 )
	{
		ret_code = initReading();
		if ( ret_code == COMMLAYER_RET_FAILED )
			return COMMLAYER_RET_FAILED;
	}
	assert( asyncReadStatus > 0 );
	if ( asyncReadStatus == 1 )
	{
		if ( !isPreReadingDone() )
			return COMMLAYER_RET_PENDING;
	}
	assert( asyncReadStatus > 1 );
	if ( asyncReadStatus == 2 )
	{
		asyncReadStatus = 0;
		return finalizeReading( msgSize, buff, maxSize);
	}
}

#else // _MSC_VER
#error not implemented
#endif
