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

bool communication_initialize()
{
#ifdef USED_AS_MASTER
	return communicationInitializeAsClient();
#else // USED_AS_MASTER
	return communicationInitializeAsServer();
#endif // USED_AS_MASTER
}

void communication_terminate()
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
#include <fcntl.h>
#include <arpa/inet.h>

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



bool communication_preinitialize()
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
	return true;
#else
	return true;
#endif
}

bool _communication_initialize()
{
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
#ifdef _MSC_VER
		int error = WSAGetLastError();
#else
		int error = errno;
#endif
		printf( "bind sock failed; error %d\n", error );
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

void _communication_terminate()
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
#ifdef _MSC_VER
	printf( "[%d] message sent; mem_h = %d, size = %d\n", GetTickCount(), mem_h, sz );
#else
	printf( "[--] message sent; mem_h = %d, size = %d\n", mem_h, sz );
#endif
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


#if (defined USED_AS_MASTER) && ( (defined USED_AS_MASTER_COMMSTACK) || (defined USED_AS_MASTER_CORE) )

const char* inet_addr_as_string_with_cl = "127.0.0.1";
int sock_with_cl;
int sock_with_cl_accepted;
struct sockaddr_in sa_self_with_cl, sa_other_with_cl;

#ifdef USED_AS_MASTER_COMMSTACK
uint16_t self_port_num_with_cl = 7665;
uint16_t other_port_num_with_cl = 7655;
#elif defined USED_AS_MASTER_CORE
uint16_t self_port_num_with_cl = 7655;
uint16_t other_port_num_with_cl = 7665;
#else
#error Unexpected configuration
#endif // USED_AS_MASTER_COMMSTACK

uint8_t buffer_in_with_cl[ BUFSIZE ];
uint16_t buffer_in_with_cl_pos;
uint8_t buffer_out_with_cl[ BUFSIZE ];

bool communication_with_comm_layer_initialize()
{
	//Zero out socket address
	memset(&sa_self_with_cl, 0, sizeof sa_self_with_cl);
	memset(&sa_other_with_cl, 0, sizeof sa_other_with_cl);

	//create an internet, datagram, socket using UDP
//	sock_with_cl = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sock_with_cl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock_with_cl) /* if socket failed to initialize, exit */
	{
		printf("Error Creating Socket\n");
		return false;
	}

	//The address is ipv4
	sa_other_with_cl.sin_family = AF_INET;
	sa_self_with_cl.sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_self_with_cl.sin_addr.s_addr = inet_addr( inet_addr_as_string_with_cl );
	sa_other_with_cl.sin_addr.s_addr = inet_addr( inet_addr_as_string_with_cl );

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_self_with_cl.sin_port = htons( self_port_num_with_cl );
	sa_other_with_cl.sin_port = htons( other_port_num_with_cl );

#ifdef USED_AS_MASTER_COMMSTACK
	if (-1 == bind(sock_with_cl, (struct sockaddr *)&sa_self_with_cl, sizeof(sa_self_with_cl)))
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
#else
		int error = errno;
#endif
		printf( "bind sock_with_cl failed; error %d\n", error );
		CLOSE_SOCKET(sock_with_cl);
		return false;
	}
/*
#ifdef _MSC_VER
    unsigned long ul = 1;
    ioctlsocket(sock_with_cl, FIONBIO, &ul);
#else
    fcntl(sock_with_cl,F_SETFL,O_NONBLOCK);
#endif
*/
	if(-1 == listen(sock_with_cl, 10))
    {
      perror("error listen failed");
      CLOSE_SOCKET(sock_with_cl);
      return false;
    }

	sock_with_cl_accepted = accept(sock_with_cl, NULL, NULL);
 
      if ( 0 > sock_with_cl_accepted )
      {
        perror("error accept failed");
        CLOSE_SOCKET(sock_with_cl);
        exit(EXIT_FAILURE);
      }

	  sock_with_cl = sock_with_cl_accepted; /*just to keep names*/

#ifdef _MSC_VER
    unsigned long ul = 1;
    ioctlsocket(sock_with_cl, FIONBIO, &ul);
#else
    fcntl(sock_with_cl,F_SETFL,O_NONBLOCK);
#endif

#else // USED_AS_MASTER_COMMSTACK
if (-1 == connect(sock_with_cl, (struct sockaddr *)&sa_other_with_cl, sizeof(sa_other_with_cl)))
    {
      perror("connect failed");
        CLOSE_SOCKET(sock_with_cl);
      return false;
    }
#endif // USED_AS_MASTER_COMMSTACK

	return true;
}

void communication_with_comm_layer_terminate()
{
	CLOSE_SOCKET(sock_with_cl);
}

bool communication_initialize()
{
#ifdef USED_AS_MASTER
#ifdef USED_AS_MASTER_CORE
	return communication_preinitialize() && communication_with_comm_layer_initialize();
#elif defined USED_AS_MASTER_COMMSTACK
	return communication_preinitialize() && communication_with_comm_layer_initialize() && _communication_initialize();
#else // 2 in 1
	return communication_preinitialize() && _communication_initialize();
#endif
#else // USED_AS_MASTER
	return communication_preinitialize() && _communication_initialize();
#endif // USED_AS_MASTER
}

void communication_terminate()
{
	_communication_terminate();
	communication_with_comm_layer_terminate();
}

uint8_t try_get_message_within_master_loop( MEMORY_HANDLE mem_h )
{
	socklen_t fromlen = sizeof(sa_other_with_cl);
	int recsize = recvfrom(sock_with_cl, (char *)(buffer_in_with_cl + buffer_in_with_cl_pos), sizeof(buffer_in_with_cl) - buffer_in_with_cl_pos, 0, (struct sockaddr *)&sa_other_with_cl, &fromlen);
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
		buffer_in_with_cl_pos += recsize;
		if ( buffer_in_with_cl_pos < 2 )
		{
			return COMMLAYER_RET_PENDING;
		}
		uint16_t sz = buffer_in_with_cl[1]; sz <<= 8; sz += buffer_in_with_cl[0];
		if ( sz > buffer_in_with_cl_pos - 2 )
		{
			return COMMLAYER_RET_PENDING;
		}
		assert( sz == buffer_in_with_cl_pos - 2 ); // TODO: try to handle
		zepto_write_block( mem_h, buffer_in_with_cl + 2, sz );
		return COMMLAYER_RET_OK;
	}

}

uint8_t try_get_message_within_master( MEMORY_HANDLE mem_h )
{
/*	socklen_t fromlen = sizeof(sa_other_with_cl);
	int recsize = recvfrom(sock_with_cl, (char *)buffer_in_with_cl, sizeof(buffer_in_with_cl), 0, (struct sockaddr *)&sa_other_with_cl, &fromlen);
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
		if ( recsize < 2 )
		zepto_write_block( mem_h, buffer_in_with_cl, recsize );
		return COMMLAYER_RET_OK;
	}*/

	buffer_in_with_cl_pos = 0;
	uint8_t ret;
	do //TODO: add delays or waiting
	{
		ret = try_get_message_within_master_loop( mem_h );
	}
	while ( ret == COMMLAYER_RET_PENDING );
	return ret;
}

uint8_t wait_for_communication_event( MEMORY_HANDLE mem_h, uint16_t timeout )
{
	printf( "wait_for_communication_event()\n" );
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
/*    FD_SET(sock_with_cl, &rfds);
#ifdef USED_AS_MASTER_COMMSTACK
    FD_SET(sock, &rfds);
#endif*/

#ifdef USED_AS_MASTER
#ifdef USED_AS_MASTER_COMMSTACK
    FD_SET(sock, &rfds);
	FD_SET(sock_with_cl, &rfds);
#else
    FD_SET(sock_with_cl, &rfds);
#endif
#else
    FD_SET(sock, &rfds);
#endif

    /* Wait */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = ((long)timeout % 1000) * 1000;

    retval = select(2, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
//		if ( error == WSAEWOULDBLOCK )
		printf( "error %d\n", error );
#else
        perror("select()");
//		int error = errno;
//		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		assert(0);
		return COMMLAYER_RET_FAILED;
	}
    else if (retval)
	{
//		assert( retval == rfds.fd_count );
//		if ( rfds.fd_array[0] == sock )
		if ( FD_ISSET(sock, &rfds) )
		{
			uint8_t ret_code = tryGetMessage( mem_h );
			if ( ret_code == COMMLAYER_RET_FAILED )
				return ret_code;
			assert( ret_code == COMMLAYER_RET_OK );
			return COMMLAYER_RET_FROM_DEV;
		}
		else 
		{
//			assert( rfds.fd_array[0] == sock_with_cl );
			assert( FD_ISSET(sock_with_cl, &rfds) );
			uint8_t ret_code = try_get_message_within_master( mem_h );
			if ( ret_code == COMMLAYER_RET_FAILED )
				return ret_code;
			assert( ret_code == COMMLAYER_RET_OK );
			return COMMLAYER_RET_FROM_CENTRAL_UNIT;
		}
	}
    else
	{
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t send_within_master( MEMORY_HANDLE mem_h )
{
	printf( "send_within_master() called...\n" );
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	uint16_t sz = zepto_parsing_remaining_bytes( &po );
	assert( sz <= BUFSIZE );
	buffer_out_with_cl[0] = (uint8_t)sz;
	buffer_out_with_cl[1] = sz >> 8;
	zepto_parse_read_block( &po, buffer_out_with_cl + 2, sz );

	
	int bytes_sent = sendto(sock_with_cl, (char*)buffer_out_with_cl, sz+2, 0, (struct sockaddr*)&sa_other_with_cl, sizeof sa_other_with_cl);
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
#ifdef _MSC_VER
	printf( "[%d] message sent within master; mem_h = %d, size = %d\n", GetTickCount(), mem_h, sz );
#else
	printf( "[--] message sent within master; mem_h = %d, size = %d\n", mem_h, sz );
#endif
	return COMMLAYER_RET_OK;
}


#ifdef USED_AS_MASTER_COMMSTACK

uint8_t send_to_central_unit( MEMORY_HANDLE mem_h )
{
	return send_within_master( mem_h );
}

#else // USED_AS_MASTER_COMMSTACK

uint8_t send_to_commm_stack( MEMORY_HANDLE mem_h )
{
	return send_within_master( mem_h );
}

#endif // USED_AS_MASTER_COMMSTACK

#else // USED_AS_MASTER

bool communication_initialize()
{
	return communication_preinitialize() && _communication_initialize();
}

void communication_terminate()
{
	_communication_terminate();
}

#endif // USED_AS_MASTER


#else
#error unknown COMM_MEAN_TYPE
#endif
