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


#define COMM_MEAN_PIPE 1
#define COMM_MEAN_SOCKET 2

//#define COMM_MEAN_TYPE COMM_MEAN_PIPE
#define COMM_MEAN_TYPE COMM_MEAN_SOCKET


#if COMM_MEAN_TYPE == COMM_MEAN_PIPE

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

uint8_t sendMessage( MEMORY_HANDLE mem_h )
{
	uint8_t buff[512];
	uint16_t msgSize;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	msgSize = zepto_parsing_remaining_bytes( &po );
	assert( msgSize <= 512 );
	zepto_parse_read_block( &po, buff, msgSize );
	printf( "[%d] message sent; mem_h = %d, size = %d\n", GetTickCount(), mem_h, msgSize );
	return sendMessage(&msgSize, buff);
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

uint8_t getMessage( MEMORY_HANDLE mem_h ) // returns when a packet received
{
	uint8_t buff[512];
	uint16_t msgSize;
	uint8_t ret = getMessage( &msgSize, buff, 512 );
	zepto_write_block( mem_h, buff, msgSize );
	return ret;
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


uint8_t tryGetMessage( MEMORY_HANDLE mem_h ) // returns immediately, but a packet reception is not guaranteed
{
	uint8_t buff[512];
	uint16_t msgSize;
	uint8_t ret = tryGetMessage( &msgSize, buff, 512 );
	if ( ret == COMMLAYER_RET_OK )
	{
		printf( "[%d] message received; mem_h = %d, size = %d\n", GetTickCount(), mem_h, msgSize );
		assert( ugly_hook_get_response_size( mem_h ) == 0 );
		zepto_write_block( mem_h, buff, msgSize );
	}
	return ret;
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


#elif COMM_MEAN_TYPE == COMM_MEAN_SOCKET



#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define CLOSE_SOCKET( x ) closesocket( x )

#else // _MSC_VER

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close() for socket 

#define CLOSE_SOCKET( x ) close( x )

#endif // _MSC_VER

const char* inet_addr_as_string = "127.0.0.1";
int sock;
struct sockaddr_in sa_self, sa_other;

#ifdef USED_AS_MASTER
uint16_t self_port_num = 7667;
uint16_t other_port_num = 7654;
#else // USED_AS_MASTER
uint16_t self_port_num = 7654;
uint16_t other_port_num = 7667;
#endif

uint8_t buffer_in[ BUFSIZE ];
uint8_t buffer_out[ BUFSIZE ];



bool communicationInitialize()
{
#ifdef _MSC_VER
	// do Windows magic
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) 
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
#endif

	//Zero out socket address
	memset(&sa_self, 0, sizeof sa_self);
	memset(&sa_other, 0, sizeof sa_other);

	//create an internet, datagram, socket using UDP
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sock) /* if socket failed to initialize, exit */
	{
		printf("Error Creating Socket\n");
		return false;
	}

	//The address is ipv4
	sa_other.sin_family = AF_INET;
	sa_self.sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_self.sin_addr.s_addr = inet_addr( inet_addr_as_string );
	sa_other.sin_addr.s_addr = inet_addr( inet_addr_as_string );

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_self.sin_port = htons( self_port_num );
	sa_other.sin_port = htons( other_port_num );

	if (-1 == bind(sock, (struct sockaddr *)&sa_self, sizeof(sa_self)))
	{
		printf( "error bind failed\n" );
		CLOSE_SOCKET(sock);
		return false;
	}

#ifdef _MSC_VER
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, &ul);
#else
    fcntl(sock,F_SETFL,O_NONBLOCK);
#endif
	return true;
}

bool communicationInitializeAsServer()
{
	return communicationInitialize();
}

bool communicationInitializeAsClient()
{
	return communicationInitialize();
}

void communicationTerminate()
{
	CLOSE_SOCKET(sock);
}

uint8_t sendMessage( MEMORY_HANDLE mem_h )
{
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	uint16_t sz = zepto_parsing_remaining_bytes( &po );
	assert( sz <= BUFSIZE );
	zepto_parse_read_block( &po, buffer_out, sz );

	
	int bytes_sent = sendto(sock, (char*)buffer_out, sz, 0, (struct sockaddr*)&sa_other, sizeof sa_other);
	if (bytes_sent < 0) 
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		printf( "Error %d sending packet\n", error );
#else
		printf("Error sending packet: %s\n", strerror(errno));
#endif
		return COMMLAYER_RET_FAILED;
	}
	return COMMLAYER_RET_OK;
}

uint8_t tryGetMessage( MEMORY_HANDLE mem_h )
{
	socklen_t fromlen = sizeof(sa_other);
	int recsize = recvfrom(sock, (char *)buffer_in, sizeof(buffer_in), 0, (struct sockaddr *)&sa_other, &fromlen);
	if (recsize < 0) 
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		if ( error == WSAEWOULDBLOCK )
#else
		int error = errno;
		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		{
			return COMMLAYER_RET_PENDING;
		}
		else
		{
			printf( "unexpected error %d received while getting message\n", error );
			return COMMLAYER_RET_FAILED;
		}
	}
	else
	{
		zepto_write_block( mem_h, buffer_in, recsize );
		return COMMLAYER_RET_OK;
	}

}



 
int xxx(void)
{
	printf("STARTING SERVER...\n");
	printf("==================\n\n");

	// do Windows magic
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// declarations
	int bytes_sent;
	int recsize;
	char buffer_out[1024];
	char buffer_in[1024];

	int sock;
	struct sockaddr_in sa_self, sa_other;

	// data initializing

	//Zero out socket address
	memset(&sa_self, 0, sizeof sa_self);
	memset(&sa_other, 0, sizeof sa_other);

	//create an internet, datagram, socket using UDP
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sock) /* if socket failed to initialize, exit */
	{
		printf("Error Creating Socket\n");
		exit(EXIT_FAILURE);
	}

	//The address is ipv4
	sa_other.sin_family = AF_INET;
	sa_self.sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_self.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa_other.sin_addr.s_addr = inet_addr("127.0.0.1");

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_self.sin_port = htons(7667);
	sa_other.sin_port = htons(7654);

	if (-1 == bind(sock, (struct sockaddr *)&sa_self, sizeof(sa_self)))
	{
		perror("error bind failed");
		closesocket(sock);
		exit(EXIT_FAILURE);
	}

	socklen_t fromlen = sizeof(sa_other);

#ifdef _WIN32
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, &ul);
#else
    fcntl(sock,F_SETFL,O_NONBLOCK);
#endif

	for ( int i=0;;i++)
	{
		printf("recv test....\n");
		recsize = recvfrom(sock, (char *)buffer_in, sizeof(buffer_in), 0, (struct sockaddr *)&sa_other, &fromlen);
		if (recsize < 0) 
		{
//			fprintf(stderr, "%s\n", strerror(errno));
//			exit(EXIT_FAILURE);
			Sleep(100);
			continue;
		}
		printf("recsize: %d\n ", recsize);
		Sleep(1);
		printf("datagram: %.*s\n", (int)recsize, buffer_in);
//		printf( "received from port: %d\n", sa_from.sin_port );

		// echo back

		//sendto(int socket, char data, int dataLength, flags, destinationAddress, int destinationStructureLength)
//		int sz_to_send = strlen(buffer);
		buffer_in[recsize] = 0;
		sprintf( buffer_out, "[%d]%s", i, buffer_in );
		bytes_sent = sendto(sock, buffer_out, strlen(buffer_out)+1, 0, (struct sockaddr*)&sa_other, sizeof sa_other);
		if (bytes_sent < 0) {
			printf("Error sending packet: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}



#elif
#error unknown COMM_MEAN_TYPE
#endif
