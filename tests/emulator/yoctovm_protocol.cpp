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

// NOTE: at present
// this implementation is fake in all parts except simulation of 
// general operations with packets
// necessary for testing of underlying levels
////////////////////////////////////////////////////////////////


#include "yoctovm_protocol.h"
#include "sagdp_protocol.h" // for packet statuses in chain

#include "test-generator.h"

#include <stdio.h> // for sprintf() in fake implementation

// Pure Testing Block
//#define MANUAL_TEST_DATA_ENTERING

#define CHAIN_MAX_SIZE 9
//bool chainContinued;
//bool isChainContinued() {return chainContinued; }
//uint16_t last_sent_id = (uint16_t)(0xFFF0) + (((uint16_t)MASTER_SLAVE_BIT) << 15 );
uint16_t last_sent_id = 0;

uint16_t currChainID;
uint16_t currChainIdBase = ( MASTER_SLAVE_BIT << 15 );
// End of Pure Testing Block 



uint8_t slave_process( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// buffer assumes to contain an input message (including First Byte)
	// FAKE structure of the buffer: First_Byte | fake_body (9-16 bytes)
	// fake body is used for external controlling of data integrity and is organized as follows:
	// chain_id (2 bytes) | chain_ini_size (2 bytes) | replying_to (2 bytes) | self_id == packet_ordinal (2 bytes) | rand_part (1-8 bytes)
	// self_id is a randomly generated 2-byte integer (for simplicity we assume that in the test environment comm peers have the same endiannes)
	// replying_to is a copy of self_id of the received packet (or 0 for the first packet ion the chain)
	// rand_part is filled with some number of '-' ended by '>'

	// get packet data
	uint8_t first_byte = buffIn[0];
	if ( ( first_byte & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_ERROR_MSG )
	{
		PRINTF( "slave_process(): ERROR MESSAGE RECEIVED IN YOCTO\n" );
		assert(0);
	}
	uint16_t chain_id = *(uint16_t*)(buffIn+1);
	uint16_t chain_ini_size = *(uint16_t*)(buffIn+3);
	uint16_t reply_to_id = *(uint16_t*)(buffIn+5);
	uint16_t self_id = *(uint16_t*)(buffIn+7);
	char tail[256];
	memcpy( tail, buffIn+9, *sizeInOut - 9 );
	tail[ *sizeInOut - 9 ] = 0;

	// print packet
	PRINTF( "Yocto: Packet received: [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", *sizeInOut, first_byte, chain_id, chain_ini_size, reply_to_id, self_id, tail );

	// test and analyze

	// size
	if ( !( *sizeInOut >= 10 && *sizeInOut <= 17 ) )
		printf( "ZEPTO: BAD PACKET RECEIVED\n", *sizeInOut );
	assert( *sizeInOut >= 10 && *sizeInOut <= 17 );

	// flags
	assert( (first_byte & 4 ) == 0 );
	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
	if ( first_byte == SAGDP_P_STATUS_FIRST )
	{
		assert( 0 == reply_to_id );
		assert( chain_id != currChainID );
		currChainID = chain_id;
	}
	else
	{
		assert( last_sent_id == reply_to_id );
		assert( chain_id == currChainID );
	}

	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
	{
//		chainContinued = false;
		currChainIdBase ++;
		return YOCTOVM_OK;
	}

	// fake implementation: should this packet be terminal?
	if ( chain_ini_size == self_id + 1 )
		first_byte = SAGDP_P_STATUS_TERMINATING;
	else
		first_byte = SAGDP_P_STATUS_INTERMEDIATE;

	// prepare outgoing packet
	reply_to_id = self_id;
	self_id++;
	last_sent_id = self_id;

	buffOut[0] = first_byte;
	*(uint16_t*)(buffOut+1) = chain_id;
	*(uint16_t*)(buffOut+3) = chain_ini_size;
	*(uint16_t*)(buffOut+5) = reply_to_id;
	*(uint16_t*)(buffOut+7) = self_id;

	uint16_t varln = 6 - self_id % 7; // 0:6
	*sizeInOut = 9+varln+1;
	buffOut[9+varln] = '>';
	while (varln--) buffOut[9+varln] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+9, *sizeInOut - 9 );
	tail[ *sizeInOut - 9 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", *sizeInOut, first_byte, chain_id, chain_ini_size, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 10 && *sizeInOut <= 17 );

	// return status
//	chainContinued = true;
	return YOCTOVM_PASS_LOWER;
}


uint8_t master_error( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	return master_start( sizeInOut, buffIn, buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
}

uint8_t master_start( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// Forms a first packet in the chain
	// for structure of the packet see comments to yocto_process()
	// Initial number of packets in the chain is currently entered manually by a tester

	uint8_t first_byte = SAGDP_P_STATUS_FIRST;

#ifdef MANUAL_TEST_DATA_ENTERING

	uint8_t chain_ini_size = 0;
	while ( chain_ini_size == 0 || chain_ini_size == '\n' )
		chain_ini_size = getchar();
	if ( chain_ini_size == 'x' )
		return YOCTOVM_FAILED;
	chain_ini_size -= '0';
	chainContinued = true;

#else // MANUAL_TEST_DATA_ENTERING

	uint16_t chain_ini_size = get_rand_val() % ( CHAIN_MAX_SIZE - 2 ) + 2;

#endif // MANUAL_TEST_DATA_ENTERING

	currChainID = ++currChainIdBase;
	uint16_t chain_id = currChainID;
	uint16_t reply_to_id = 0;
	uint16_t self_id = 1;
	last_sent_id = self_id;


	// packet ids
/*	last_sent_id += 0x10;
	last_sent_id &= (uint16_t)(0xFFF0);
	last_sent_id ++; // thus starting from 1 in the last xex digit
	uint16_t reply_to_id = 0;
	uint16_t self_id = last_sent_id;*/

	// prepare outgoing packet
	buffOut[0] = first_byte;
	*(uint16_t*)(buffOut+1) = chain_id;
	*(uint16_t*)(buffOut+3) = chain_ini_size;
	*(uint16_t*)(buffOut+5) = reply_to_id;
	*(uint16_t*)(buffOut+7) = self_id;

	uint16_t varln = 6 - self_id % 7; // 0:6
	*sizeInOut = 9+varln+1;
	buffOut[9+varln] = '>';
	while (varln--) buffOut[9+varln] = '-';

	// print outgoing packet
	char tail[256];
	memcpy( tail, buffOut+9, *sizeInOut - 9 );
	tail[ *sizeInOut - 9 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", *sizeInOut, first_byte, chain_id, chain_ini_size, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 10 && *sizeInOut <= 17 );

	// return status
	return YOCTOVM_PASS_LOWER;
}
/*
//bool start_over_once = true;
bool start_over_once = false;
bool statck_reset = false;
*/
uint8_t master_continue( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// by now master_continue() does the same as yocto_process


	// get packet data
	uint8_t first_byte = buffIn[0];
	if ( ( first_byte & ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING ) ) == SAGDP_P_STATUS_ERROR_MSG )
	{
		PRINTF( "master_continue(): ERROR MESSAGE RECEIVED IN YOCTO\n" );
		return master_start( sizeInOut, buffIn, buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
	}
	uint16_t chain_id = *(uint16_t*)(buffIn+1);
	uint16_t chain_ini_size = *(uint16_t*)(buffIn+3);
	uint16_t reply_to_id = *(uint16_t*)(buffIn+5);
	uint16_t self_id = *(uint16_t*)(buffIn+7);
	char tail[256];
	memcpy( tail, buffIn+9, *sizeInOut - 9 );
	tail[ *sizeInOut - 9 ] = 0;

	// print packet
	PRINTF( "Yocto: Packet received: [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", *sizeInOut, first_byte, chain_id, chain_ini_size, reply_to_id, self_id, tail );

	// test and analyze

	// size
	if ( !( *sizeInOut >= 10 && *sizeInOut <= 17 ) )
		printf( "ZEPTO: BAD PACKET RECEIVED\n", *sizeInOut );
	assert( *sizeInOut >= 10 && *sizeInOut <= 17 );

	// flags
	assert( (first_byte & 4 ) == 0 );
	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits

	if ( first_byte == SAGDP_P_STATUS_FIRST )
	{
		assert( 0 == reply_to_id );
		assert( chain_id != currChainID );
		currChainID = chain_id;
	}
	else
	{
		assert( last_sent_id == reply_to_id );
		assert( chain_id == currChainID );
	}

	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
	{
//		chainContinued = false;
		currChainIdBase ++;
		return YOCTOVM_OK;
	}

/*	if ( start_over_once )
	{
		if ( second_byte == 3 || second_byte == 4 )
		{
			if (!statck_reset)
			{
				statck_reset = true;
				return YOCTOVM_RESET_STACK;
			}
			else
			{
				start_over_once = false;
				return master_start( sizeInOut, buffIn, buffOut );
			}
		}
	}


	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
	{
		return YOCTOVM_OK;
//		return master_start( sizeInOut, buffIn, buffOut );
	}*/

	// fake implementation: should this packet be terminal?
	if ( chain_ini_size == self_id + 1 )
		first_byte = SAGDP_P_STATUS_TERMINATING;
	else
		first_byte = SAGDP_P_STATUS_INTERMEDIATE;

	// prepare outgoing packet
	reply_to_id = self_id;
	self_id++;
	last_sent_id = self_id;

	buffOut[0] = first_byte;
	*(uint16_t*)(buffOut+1) = chain_id;
	*(uint16_t*)(buffOut+3) = chain_ini_size;
	*(uint16_t*)(buffOut+5) = reply_to_id;
	*(uint16_t*)(buffOut+7) = self_id;

	uint16_t varln = 6 - self_id % 7; // 0:6
	*sizeInOut = 9+varln+1;
	buffOut[9+varln] = '>';
	while (varln--) buffOut[9+varln] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+9, *sizeInOut - 9 );
	tail[ *sizeInOut - 9 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d bytes]  [%d][0x%04x][0x%04x][0x%04x][0x%04x]%s\n", *sizeInOut, first_byte, chain_id, chain_ini_size, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 10 && *sizeInOut <= 17 );

	// return status
	return first_byte == SAGDP_P_STATUS_TERMINATING ? YOCTOVM_PASS_LOWER_THEN_IDLE : YOCTOVM_PASS_LOWER;
}
