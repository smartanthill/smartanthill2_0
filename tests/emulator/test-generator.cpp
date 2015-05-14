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
#include "test-generator.h"
#include <stdlib.h> // for get_rand_val()

#define MAX_IPACKETS_TO_STORE ((uint16_t)5)
#define PACKET_MAX_SIZE ((uint16_t)514)
uint8_t incomingPackets[ MAX_IPACKETS_TO_STORE * PACKET_MAX_SIZE ];
uint8_t outgoingPackets[ MAX_IPACKETS_TO_STORE * PACKET_MAX_SIZE ];


uint8_t packetOnHold[ PACKET_MAX_SIZE ];
bool isPacketOnHold = false;
bool holdRequested = false;

void tester_registerIncomingPacket( const uint8_t* packet, uint16_t size );
void tester_registerOutgoingPacket( const uint8_t* packet, uint16_t size );
bool tester_shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size );
bool tester_shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size );

bool tester_holdOutgoingPacket( const uint8_t* packet, const uint16_t* size );
bool tester_releaseOutgoingPacket( uint8_t* packet, uint16_t* size );
bool tester_holdPacketOnRequest( const uint8_t* packet, const uint16_t* size );



void tester_registerIncomingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	tester_registerIncomingPacket( buff, packet_size );
}

void tester_registerOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	tester_registerOutgoingPacket( buff, packet_size );
}

bool tester_shouldInsertIncomingPacket( REQUEST_REPLY_HANDLE mem_h )
{
//	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = tester_shouldInsertIncomingPacket( buff, &packet_size );
	if ( !ret ) return false;
	assert( ugly_hook_get_response_size( mem_h ) == 0 );
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}

bool tester_shouldInsertOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
//	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = tester_shouldInsertOutgoingPacket( buff, &packet_size );
	if ( !ret ) return false;
	assert( ugly_hook_get_response_size( mem_h ) == 0 );
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}



bool tester_holdOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
//	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	bool ret = tester_holdOutgoingPacket( buff, &packet_size );
	if ( ret )
		zepto_response_to_request( mem_h );
	return ret;
}

bool tester_releaseOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
//	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = tester_releaseOutgoingPacket( buff, &packet_size );
	if ( !ret ) return false;
	assert( ugly_hook_get_response_size( mem_h ) == 0 );
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}

bool tester_holdPacketOnRequest( REQUEST_REPLY_HANDLE mem_h )
{
//	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
//	return tester_holdOutgoingPacket( buff, &packet_size );
	bool ret = tester_holdPacketOnRequest( buff, &packet_size );
	if ( ret )
		zepto_response_to_request( mem_h );
	return ret;
}










bool tester_holdOutgoingPacket( const uint8_t* packet, const uint16_t* size )
{
	if ( isPacketOnHold )
		return false;
	*(uint16_t*)packetOnHold = *size;
	memcpy( incomingPackets + 2, packet, *size );
	isPacketOnHold = true;
	return true;
}

bool tester_isOutgoingPacketOnHold()
{
	return false;
	return isPacketOnHold;
}

bool tester_releaseOutgoingPacket( uint8_t* packet, uint16_t* size )
{
	if ( !isPacketOnHold )
		return false;
	*size = *(uint16_t*)packetOnHold;
	memcpy( packet, incomingPackets + 2, *size );
	isPacketOnHold = false;
	return *size != 0;
}

void tester_requestHoldingPacket()
{
	assert( !isPacketOnHold );
	holdRequested = true;
}

bool tester_holdPacketOnRequest( const uint8_t* packet, const uint16_t* size )
{
	if ( !holdRequested )
		return false;
	holdRequested = false;
	assert( !isPacketOnHold );
	return tester_holdOutgoingPacket( packet, size );
}




uint16_t tester_get_rand_val()
{
	return (uint16_t)( rand() );
}

void tester_registerIncomingPacket( const uint8_t* packet, uint16_t size )
{
	assert( size <= PACKET_MAX_SIZE );
	for ( int8_t i=MAX_IPACKETS_TO_STORE-1; i; i-- )
		memcpy( incomingPackets + i * PACKET_MAX_SIZE, incomingPackets + (i-1)*PACKET_MAX_SIZE, PACKET_MAX_SIZE );
	*(uint16_t*)incomingPackets = size;
	memcpy( incomingPackets + 2, packet, size );
}

void tester_registerOutgoingPacket( const uint8_t* packet, uint16_t size )
{
	assert( size <= PACKET_MAX_SIZE );
	for ( int8_t i=MAX_IPACKETS_TO_STORE-1; i; i-- )
		memcpy( outgoingPackets + i * PACKET_MAX_SIZE, outgoingPackets + (i-1)*PACKET_MAX_SIZE, PACKET_MAX_SIZE );
	*(uint16_t*)outgoingPackets = size;
	memcpy( outgoingPackets + 2, packet, size );
}



bool tester_shouldDropIncomingPacket()
{
//	return false;
	return tester_get_rand_val() % 8 == 0; // rate selection
}

bool tester_shouldDropOutgoingPacket()
{
//	return false;
	return tester_get_rand_val() % 8 == 0; // rate selection
}



bool tester_shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size )
{
	if ( tester_get_rand_val() % 8 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = tester_get_rand_val() % ( MAX_IPACKETS_TO_STORE - 1 ) + 1;
	*size = *(uint16_t*)( incomingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, incomingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}

bool tester_shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size )
{
	if ( tester_get_rand_val() % 8 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = tester_get_rand_val() % ( MAX_IPACKETS_TO_STORE - 1 ) + 1;
	*size = *(uint16_t*)( outgoingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, outgoingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}


#ifdef _MSC_VER

#include <windows.h>

HANDLE hSyncEvent;
const char* syncEventName = "sa_testing_sync_event";

void tester_initTestSystem()
{
	hSyncEvent = CreateEventA( NULL, TRUE, TRUE, syncEventName );
	assert( hSyncEvent != NULL );
	memset( packetOnHold, 0, sizeof( packetOnHold ) );
	isPacketOnHold = false;
}

void tester_freeTestSystem()
{
	CloseHandle( hSyncEvent );
}
/*
void requestSyncExec()
{
	ResetEvent( hSyncEvent );
}

void allowSyncExec()
{
	SetEvent( hSyncEvent );
}

void waitToProceed()
{
	WaitForSingleObject( hSyncEvent, 10000 );	// should be "infinitely", but let's be practical
}
*/
void justWaitSec( uint16_t durationSec )
{
	Sleep( durationSec * 1000 );
}

void justWaitMSec( uint16_t durationMSec )
{
	Sleep( durationMSec );
}

#else
#include <time.h>
void tester_initTestSystem()
{
}

void tester_freeTestSystem()
{
}

void justWaitSec( uint16_t durationSec )
{
	timespec ts;
	ts.tv_sec = durationSec;
	ts.tv_nsec = 0;
	nanosleep( &ts, NULL );
}

void justWaitMSec( uint16_t durationMSec )
{
	timespec ts;
	ts.tv_sec = durationMSec / 1000;
	ts.tv_nsec = ( durationMSec % 1000 ) * 1000000;
	nanosleep( &ts, NULL );
}

#endif
