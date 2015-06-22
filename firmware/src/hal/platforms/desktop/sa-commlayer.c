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

#include "../../sa-commlayer.h"
#include "../../hal-waiting.h"
#include <stdio.h>

#define MAX_PACKET_SIZE 50


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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
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



bool communication_preinitialize()
{
#ifdef _MSC_VER
	// do Windows magic
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		ZEPTO_DEBUG_PRINTF_2("WSAStartup failed with error: %d\n", iResult);
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
		ZEPTO_DEBUG_PRINTF_1("Error Creating Socket\n");
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
		ZEPTO_DEBUG_PRINTF_2( "bind sock failed; error %d\n", error );
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

uint8_t send_message( MEMORY_HANDLE mem_h )
{
	uint16_t sz = memory_object_get_request_size( mem_h );
	ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
	uint8_t* buff = memory_object_get_request_ptr( mem_h );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	int bytes_sent = sendto(sock, (char*)buff, sz, 0, (struct sockaddr*)&sa_other, sizeof sa_other);
	if (bytes_sent < 0)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		ZEPTO_DEBUG_PRINTF_2( "Error %d sending packet\n", error );
#else
		ZEPTO_DEBUG_PRINTF_2("Error sending packet: %s\n", strerror(errno));
#endif
		return COMMLAYER_RET_FAILED;
	}
#ifdef _MSC_VER
	ZEPTO_DEBUG_PRINTF_4( "[%d] message sent; mem_h = %d, size = %d\n", GetTickCount(), mem_h, sz );
#else
	ZEPTO_DEBUG_PRINTF_3( "[--] message sent; mem_h = %d, size = %d\n", mem_h, sz );
#endif
	return COMMLAYER_RET_OK;
}

uint8_t try_get_message( MEMORY_HANDLE mem_h )
{
	// It is assumed here that the system must be able to receive a packet up to MAX_PACKET_SIZE BYTES. Thus we first request this amount of memory, and then release unnecessary part

	// do cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );
	uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

	socklen_t fromlen = sizeof(sa_other);
//	int recsize = recvfrom(sock, (char *)buffer_in, sizeof(buffer_in), 0, (struct sockaddr *)&sa_other, &fromlen);
	uint16_t recsize = recvfrom(sock, (char *)buff, MAX_PACKET_SIZE, 0, (struct sockaddr *)&sa_other, &fromlen);
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
			ZEPTO_DEBUG_PRINTF_2( "unexpected error %d received while getting message\n", error );
			return COMMLAYER_RET_FAILED;
		}
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( recsize && recsize <= MAX_PACKET_SIZE );
//		zepto_write_block( mem_h, buffer_in, recsize );
		memory_object_response_to_request( mem_h );
		memory_object_cut_and_make_response( mem_h, 0, recsize );
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

uint16_t buffer_in_with_cl_pos;

bool communication_with_comm_layer_initialize()
{
	//Zero out socket address
	memset(&sa_self_with_cl, 0, sizeof sa_self_with_cl);
	memset(&sa_other_with_cl, 0, sizeof sa_other_with_cl);

	//create an internet, datagram, socket using UDP
	sock_with_cl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock_with_cl) /* if socket failed to initialize, exit */
	{
		ZEPTO_DEBUG_PRINTF_1("Error Creating Socket\n");
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
		ZEPTO_DEBUG_PRINTF_2( "bind sock_with_cl failed; error %d\n", error );
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

uint8_t try_get_packet_within_master_loop( uint8_t* buff, uint16_t sz )
{
	socklen_t fromlen = sizeof(sa_other_with_cl);
	int recsize = recvfrom(sock_with_cl, (char *)(buff + buffer_in_with_cl_pos), sz - buffer_in_with_cl_pos, 0, (struct sockaddr *)&sa_other_with_cl, &fromlen);
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
			ZEPTO_DEBUG_PRINTF_2( "unexpected error %d received while getting message\n", error );
			return COMMLAYER_RET_FAILED;
		}
	}
	else
	{
		buffer_in_with_cl_pos += recsize;
		if ( buffer_in_with_cl_pos < sz )
		{
			return COMMLAYER_RET_PENDING;
		}
		return COMMLAYER_RET_OK;
	}

}

uint8_t try_get_packet_size_within_master_loop( uint8_t* buff )
{
	socklen_t fromlen = sizeof(sa_other_with_cl);
	int recsize = recvfrom(sock_with_cl, (char *)(buff + buffer_in_with_cl_pos), 2 - buffer_in_with_cl_pos, 0, (struct sockaddr *)&sa_other_with_cl, &fromlen);
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
			ZEPTO_DEBUG_PRINTF_2( "unexpected error %d received while getting message\n", error );
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
		return COMMLAYER_RET_OK;
	}

}

uint8_t try_get_message_within_master( MEMORY_HANDLE mem_h )
{
	// do cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );
	uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

	buffer_in_with_cl_pos = 0;
	uint8_t ret;

	do //TODO: add delays or some waiting
	{
		ret = try_get_packet_size_within_master_loop( buff );
	}
	while ( ret == COMMLAYER_RET_PENDING );
	if ( ret != COMMLAYER_RET_OK )
		return ret;
	uint16_t sz = buff[1]; sz <<= 8; sz += buff[0];

	buffer_in_with_cl_pos = 0;
	do //TODO: add delays or some waiting
	{
		ret = try_get_packet_within_master_loop( buff, sz );
	}
	while ( ret == COMMLAYER_RET_PENDING );

	memory_object_response_to_request( mem_h );
	memory_object_cut_and_make_response( mem_h, 0, sz );

	return ret;
}

uint8_t send_within_master( MEMORY_HANDLE mem_h )
{
	ZEPTO_DEBUG_PRINTF_1( "send_within_master() called...\n" );

	uint16_t sz = memory_object_get_request_size( mem_h );
	memory_object_request_to_response( mem_h );
	ZEPTO_DEBUG_ASSERT( sz == memory_object_get_response_size( mem_h ) );
	ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
	uint8_t* buff = memory_object_prepend( mem_h, 2 );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	buff[0] = (uint8_t)sz;
	buff[1] = sz >> 8;
	int bytes_sent = sendto(sock_with_cl, (char*)buff, sz+2, 0, (struct sockaddr*)&sa_other_with_cl, sizeof sa_other_with_cl);
	// do full cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );


	if (bytes_sent < 0)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		ZEPTO_DEBUG_PRINTF_2( "Error %d sending packet\n", error );
#else
		ZEPTO_DEBUG_PRINTF_2("Error sending packet: %s\n", strerror(errno));
#endif
		return COMMLAYER_RET_FAILED;
	}
#ifdef _MSC_VER
	ZEPTO_DEBUG_PRINTF_4( "[%d] message sent within master; mem_h = %d, size = %d\n", GetTickCount(), mem_h, sz );
#else
	ZEPTO_DEBUG_PRINTF_3( "[--] message sent within master; mem_h = %d, size = %d\n", mem_h, sz );
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

//uint8_t wait_for_communication_event( MEMORY_HANDLE mem_h, uint16_t timeout )
uint8_t wait_for_communication_event( unsigned int timeout )
{
	ZEPTO_DEBUG_PRINTF_1( "wait_for_communication_event()\n" );
    fd_set rfds;
    struct timeval tv;
    int retval;
	int fd_cnt;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);

#ifdef USED_AS_MASTER
#ifdef USED_AS_MASTER_COMMSTACK
    FD_SET(sock, &rfds);
	FD_SET(sock_with_cl, &rfds);
	fd_cnt = sock > sock_with_cl ? sock + 1 : sock_with_cl + 1;
#else
    FD_SET(sock_with_cl, &rfds);
	fd_cnt = sock_with_cl + 1;
#endif
#else
    FD_SET(sock, &rfds);
	fd_cnt = sock + 1;
#endif

    /* Wait */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = ((long)timeout % 1000) * 1000;

    retval = select(fd_cnt, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
//		if ( error == WSAEWOULDBLOCK )
		ZEPTO_DEBUG_PRINTF_2( "error %d\n", error );
#else
        perror("select()");
//		int error = errno;
//		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		ZEPTO_DEBUG_ASSERT(0);
		return COMMLAYER_RET_FAILED;
	}
    else if (retval)
	{
		if ( FD_ISSET(sock, &rfds) )
		{
/*			uint8_t ret_code = try_get_message( mem_h );
			if ( ret_code == COMMLAYER_RET_FAILED )
				return ret_code;
			ZEPTO_DEBUG_ASSERT( ret_code == COMMLAYER_RET_OK );*/
			return COMMLAYER_RET_FROM_DEV;
		}
#ifdef USED_AS_MASTER
		else
		{
//			ZEPTO_DEBUG_ASSERT( rfds.fd_array[0] == sock_with_cl );
			ZEPTO_DEBUG_ASSERT( FD_ISSET(sock_with_cl, &rfds) );
/*			uint8_t ret_code = try_get_message_within_master( mem_h );
			if ( ret_code == COMMLAYER_RET_FAILED )
				return ret_code;
			ZEPTO_DEBUG_ASSERT( ret_code == COMMLAYER_RET_OK );*/
			return COMMLAYER_RET_FROM_CENTRAL_UNIT;
		}
#endif // USED_AS_MASTER
	}
    else
	{
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t wait_for_timeout( unsigned int timeout)
{
    struct timeval tv;
    int retval;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = ((long)timeout % 1000) * 1000;

    retval = select(0, NULL, NULL, NULL, &tv);

    if (retval == -1)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		ZEPTO_DEBUG_PRINTF_2( "error %d\n", error );
#else
        perror("select()");
//		int error = errno;
//		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		ZEPTO_DEBUG_ASSERT(0);
		return COMMLAYER_RET_FAILED;
	}
    else
	{
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t hal_wait_for( waiting_for* wf )
{
	unsigned int timeout = wf->wait_time.high_t;
	timeout <<= 16;
	timeout += wf->wait_time.low_t;
	uint8_t ret_code;
	ZEPTO_DEBUG_ASSERT( wf->wait_legs == 0 ); // not implemented
	ZEPTO_DEBUG_ASSERT( wf->wait_i2c == 0 ); // not implemented
	if ( wf->wait_packet )
	{
		ret_code = wait_for_communication_event( timeout );
		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_DEV: return WAIT_RESULTED_IN_PACKET; break;
			case COMMLAYER_RET_TIMEOUT: return WAIT_RESULTED_IN_TIMEOUT; break;
			case COMMLAYER_RET_FAILED: return WAIT_RESULTED_IN_FAILURE; break;
			default: return WAIT_RESULTED_IN_FAILURE;
		}
	}
	else
	{
		ret_code = wait_for_timeout( timeout );
		switch ( ret_code )
		{
			case COMMLAYER_RET_TIMEOUT: return WAIT_RESULTED_IN_TIMEOUT; break;
			default: return WAIT_RESULTED_IN_FAILURE;
		}
	}
}

void keep_transmitter_on( bool keep_on )
{
	// TODO: add reasonable implementation
}
