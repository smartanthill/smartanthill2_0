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

uint8_t sendMessage(uint16_t* msgSize, const uint8_t * buff)
{
	BOOL   fSuccess = FALSE;
	DWORD  cbWritten;
	uint8_t sizePacked[2];
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

uint8_t getMessage(uint16_t* msgSize, uint8_t * buff, int maxSize) // returns when a packet received
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

uint8_t finalizeReading(uint16_t* msgSize, uint8_t * buff, int maxSize)
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

uint8_t tryGetMessage(uint16_t* msgSize, uint8_t * buff, int maxSize) // returns immediately, but a packet reception is not guaranteed
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
