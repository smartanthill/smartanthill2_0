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
#include "test-generator.h"
#include <stdlib.h> // for rand()

#define MAX_IPACKETS_TO_STORE ((uint16_t)5)
#define PACKET_MAX_SIZE ((uint16_t)514)
uint8_t incomingPackets[ MAX_IPACKETS_TO_STORE * PACKET_MAX_SIZE ];
uint8_t outgoingPackets[ MAX_IPACKETS_TO_STORE * PACKET_MAX_SIZE ];

#if !defined USED_AS_MASTER
bool slaveStartsSequence = true;
bool startSequence()
{
	return slaveStartsSequence;
}
#endif

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
//	return false;
	return rand() % 2 == 0; // rate selection
}

bool shouldDropOutgoingPacket()
{
//	return false;
	return rand() % 2 == 0; // rate selection
}



bool shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size )
{
	if ( rand() % 2 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = rand() % MAX_IPACKETS_TO_STORE;
	*size = *(uint16_t*)( incomingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, incomingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}

bool shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size )
{
	if ( rand() % 2 != 0 ) // rate selection
		return false;

	// select one of saved incoming packets
	uint8_t sel_packet = rand() % MAX_IPACKETS_TO_STORE;
	*size = *(uint16_t*)( outgoingPackets + PACKET_MAX_SIZE * sel_packet );
	memcpy( packet, outgoingPackets + PACKET_MAX_SIZE * sel_packet + 2, *size );
	return *size != 0;
}

void insertIncomingPacket()
{
	if ( rand() % 2 != 0 ) // rate selection
		return;

	// select one of saved incoming packets
	uint8_t sel_packet = rand() % MAX_IPACKETS_TO_STORE;
	uint16_t size = *(uint16_t*)( incomingPackets + PACKET_MAX_SIZE * sel_packet );
	if ( size != 0 )
		sendMessage( &size, incomingPackets + PACKET_MAX_SIZE * sel_packet + 2 );
}

void insertOutgoingPacket()
{
	if ( rand() % 2 != 0 ) // rate selection
		return;

	// select one of saved incoming packets
	uint8_t sel_packet = rand() % MAX_IPACKETS_TO_STORE;
	uint16_t size = *(uint16_t*)( outgoingPackets + PACKET_MAX_SIZE * sel_packet );
	if ( size != 0 )
		sendMessage( &size, outgoingPackets + PACKET_MAX_SIZE * sel_packet + 2 );
}



#ifdef _MSC_VER

#include <windows.h>

HANDLE hSyncEvent;
const char* syncEventName = "sa_testing_sync_event";

void initTestSystem()
{
	hSyncEvent = CreateEventA( NULL, TRUE, TRUE, syncEventName );
	assert( hSyncEvent != NULL );
}

void freeTestSystem()
{
	CloseHandle( hSyncEvent );
}

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

void justWait( uint16_t durationSec )
{
	Sleep( durationSec * 1000 );
}

#else
#error not implemented
#endif