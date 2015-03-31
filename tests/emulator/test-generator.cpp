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

void registerIncomingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	registerIncomingPacket( buff, packet_size );
}

void registerOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	registerOutgoingPacket( buff, packet_size );
}

bool shouldInsertIncomingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = shouldInsertIncomingPacket( buff, &packet_size );
	if ( !ret ) return false;
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}

bool shouldInsertOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = shouldInsertOutgoingPacket( buff, &packet_size );
	if ( !ret ) return false;
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}



bool holdOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	return holdOutgoingPacket( buff, &packet_size );
}

bool releaseOutgoingPacket( REQUEST_REPLY_HANDLE mem_h )
{
	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	bool ret = releaseOutgoingPacket( buff, &packet_size );
	if ( !ret ) return false;
	zepto_write_block( mem_h, buff, packet_size );
	return true;
}

bool holdPacketOnRequest( REQUEST_REPLY_HANDLE mem_h )
{
	return false;
	uint8_t buff[ PACKET_MAX_SIZE ];
	uint16_t packet_size;
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	packet_size = zepto_parsing_remaining_bytes( &po );
	assert( packet_size <= PACKET_MAX_SIZE );
	zepto_parse_read_block( &po, buff, packet_size );
	return holdOutgoingPacket( buff, &packet_size );
}










bool holdOutgoingPacket( const uint8_t* packet, const uint16_t* size )
{
	if ( isPacketOnHold )
		return false;
	*(uint16_t*)packetOnHold = *size;
	memcpy( incomingPackets + 2, packet, *size );
	isPacketOnHold = true;
	return true;
}

bool isOutgoingPacketOnHold()
{
	return false;
	return isPacketOnHold;
}

bool releaseOutgoingPacket( uint8_t* packet, uint16_t* size )
{
	if ( !isPacketOnHold )
		return false;
	*size = *(uint16_t*)packetOnHold;
	memcpy( packet, incomingPackets + 2, *size );
	isPacketOnHold = false;
	return true;
}

void requestHoldingPacket()
{
	assert( !isPacketOnHold );
	holdRequested = true;
}

bool holdPacketOnRequest( const uint8_t* packet, const uint16_t* size )
{
	if ( !holdRequested )
		return false;
	holdRequested = false;
	assert( !isPacketOnHold );
	return holdOutgoingPacket( packet, size );
}




uint16_t get_rand_val()
{
	return (uint16_t)( rand() );
}

void registerIncomingPacket( const uint8_t* packet, uint16_t size )
{
	assert( size <= PACKET_MAX_SIZE );
	for ( int8_t i=MAX_IPACKETS_TO_STORE-1; i; i-- )
		memcpy( incomingPackets + i * PACKET_MAX_SIZE, incomingPackets + (i-1)*PACKET_MAX_SIZE, PACKET_MAX_SIZE );
	*(uint16_t*)incomingPackets = size;
	memcpy( incomingPackets + 2, packet, size );
}

void registerOutgoingPacket( const uint8_t* packet, uint16_t size )
{
	assert( size <= PACKET_MAX_SIZE );
	for ( int8_t i=MAX_IPACKETS_TO_STORE-1; i; i-- )
		memcpy( outgoingPackets + i * PACKET_MAX_SIZE, outgoingPackets + (i-1)*PACKET_MAX_SIZE, PACKET_MAX_SIZE );
	*(uint16_t*)outgoingPackets = size;
	memcpy( outgoingPackets + 2, packet, size );
}



bool shouldDropIncomingPacket()
{
	return false;
//	return get_rand_val() % 8 == 0; // rate selection
}

bool shouldDropOutgoingPacket()
{
	return false;
//	return get_rand_val() % 8 == 0; // rate selection
}



bool shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size )
{
//	if ( get_rand_val() % 8 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = get_rand_val() % ( MAX_IPACKETS_TO_STORE - 1 ) + 1;
	*size = *(uint16_t*)( incomingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, incomingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}

bool shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size )
{
//	if ( get_rand_val() % 8 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = get_rand_val() % ( MAX_IPACKETS_TO_STORE - 1 ) + 1;
	*size = *(uint16_t*)( outgoingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, outgoingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}
/*
void insertIncomingPacket()
{
	if ( get_rand_val() % 2 != 0 ) // rate selection
		return;

	// select one of saved incoming packets
	uint8_t sel_packet = get_rand_val() % MAX_IPACKETS_TO_STORE;
	uint16_t size = *(uint16_t*)( incomingPackets + PACKET_MAX_SIZE * sel_packet );
	if ( size != 0 )
		sendMessage( &size, incomingPackets + PACKET_MAX_SIZE * sel_packet + 2 );
}

void insertOutgoingPacket()
{
	if ( get_rand_val() % 2 != 0 ) // rate selection
		return;

	// select one of saved incoming packets
	uint8_t sel_packet = get_rand_val() % MAX_IPACKETS_TO_STORE;
	uint16_t size = *(uint16_t*)( outgoingPackets + PACKET_MAX_SIZE * sel_packet );
	if ( size != 0 )
		sendMessage( &size, outgoingPackets + PACKET_MAX_SIZE * sel_packet + 2 );
}
*/


#ifdef _MSC_VER

#include <windows.h>

HANDLE hSyncEvent;
const char* syncEventName = "sa_testing_sync_event";

void initTestSystem()
{
	hSyncEvent = CreateEventA( NULL, TRUE, TRUE, syncEventName );
	assert( hSyncEvent != NULL );
	memset( packetOnHold, 0, sizeof( packetOnHold ) );
	isPacketOnHold = false;
}

void freeTestSystem()
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
void justWait( uint16_t durationSec )
{
	Sleep( durationSec * 1000 );
}

#else
#error not implemented
#endif