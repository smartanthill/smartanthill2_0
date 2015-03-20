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

#if !defined __TEST_GENERATOR_H__
#define __TEST_GENERATOR_H__

// initialization
void initTestSystem();
void freeTestSystem();

// common testing calls
uint16_t get_rand_val();

// comm layer hooks
void registerIncomingPacket( const uint8_t* packet, uint16_t size );
void registerOutgoingPacket( const uint8_t* packet, uint16_t size );
bool shouldDropIncomingPacket();
bool shouldDropOutgoingPacket();
bool shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size );
bool shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size );
/*
void insertIncomingPacket();
void insertOutgoingPacket();
*/

bool holdOutgoingPacket( const uint8_t* packet, const uint16_t* size );
bool isOutgoingPacketOnHold();
bool releaseOutgoingPacket( uint8_t* packet, uint16_t* size );
void requestHoldingPacket();
bool holdPacketOnRequest( const uint8_t* packet, const uint16_t* size );

// sync hooks
/*void requestSyncExec();
void allowSyncExec();
void waitToProceed();*/
void justWait( uint16_t durationSec );


#endif // __TEST_GENERATOR_H__