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

#include <stdio.h> // for sprintf() in fake implementation

// packet statuses
#define SAGDP_P_STATUS_INTERMEDIATE 0
#define SAGDP_P_STATUS_FIRST 1
#define SAGDP_P_STATUS_TERMINATING 2

// Pure Testing Block
bool chainContinued;
bool isChainContinued() {return chainContinued; }
uint16_t last_sent_id = (uint16_t)(0xFFF0) + (((uint16_t)MASTER_SLAVE_BIT) << 15 );
// End of Pure Testing Block 



uint8_t slave_process( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// buffer assumes to contain an input message (including First Byte)
	// FAKE structure of the buffer: First_Byte | remaining_packet_cnt (1 byte) | fake_body (9-12 bytes)
	// fake body is used for external controlling of data integrity and is organized as follows:
	// replying_to (2 bytes) | self_id (2 bytes) | rand_part (1-8 bytes)
	// self_id is a randomly generated 2-byte integer (for simplicity we assume that in the test environment comm peers have the same endiannes)
	// replying_to is a copy of self_id of the received packet (or 0 for the first packet ion the chain)
	// rand_part is filled with some number of '-' ended by '>'

	// get packet data
	uint8_t first_byte = buffIn[0];
	uint8_t second_byte = buffIn[1];
	uint16_t reply_to_id = *(uint16_t*)(buffIn+2);
	uint16_t self_id = *(uint16_t*)(buffIn+4);

	// print received packet
	if ( !(*sizeInOut >= 7 && *sizeInOut <= 14) )
		printf( "yocto: Unexpected incoming packet size %d\n", *sizeInOut );

	char tail[256];
	memcpy( tail, buffIn+6, *sizeInOut - 6 );
	tail[ *sizeInOut - 6 ] = 0;
	PRINTF( "Yocto: Packet received: [%d][%d][0x%04x][0x%04x]%s\n", first_byte, second_byte, reply_to_id, self_id, tail );

	// check packet data
	assert( *sizeInOut >= 7 && *sizeInOut <= 14 );

	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
	if ( first_byte == SAGDP_P_STATUS_FIRST )
		assert( 0 == reply_to_id );
	else
		assert( last_sent_id == reply_to_id );

	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
	{
		chainContinued = false;
		return YOCTOVM_OK;
	}

	// fake implementation: should this packet be terminal?
	if ( second_byte <= 1 )
		first_byte = SAGDP_P_STATUS_TERMINATING;
	else
		first_byte = SAGDP_P_STATUS_INTERMEDIATE;
	second_byte--;

	// prepare outgoing packet
	reply_to_id = self_id;
	self_id++;
	last_sent_id = self_id;

	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	*(uint16_t*)(buffOut+2) = reply_to_id;
	*(uint16_t*)(buffOut+4) = self_id;

	uint16_t varln = 6 - self_id % 7;
	*sizeInOut = 6+varln+1;
	buffOut[6+varln] = '>';
	while (varln--) buffOut[6+varln] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+6, *sizeInOut - 6 );
	tail[ *sizeInOut - 6 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%04x][0x%04x]%s\n", first_byte, second_byte, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 7 && *sizeInOut <= 14 );

	// return status
	chainContinued = true;
	return YOCTOVM_PASS_LOWER;
}



uint8_t master_start( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// Forms a first packet in the chain
	// for structure of the packet see comments to yocto_process()
	// Initial number of packets in the chain is currently entered manually by a tester

	uint8_t first_byte = SAGDP_P_STATUS_FIRST;
	uint8_t second_byte = 0;
	while ( second_byte == 0 || second_byte == '\n' )
		second_byte = getchar();
	if ( second_byte == 'x' )
		return YOCTOVM_FAILED;
	second_byte -= '0';
	chainContinued = true;

	// packet ids
	last_sent_id += 0x10;
	last_sent_id &= (uint16_t)(0xFFF0);
	last_sent_id ++; // thus starting from 1 in the last xex digit
	uint16_t reply_to_id = 0;
	uint16_t self_id = last_sent_id;

	// prepare outgoing packet
	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	*(uint16_t*)(buffOut+2) = reply_to_id;
	*(uint16_t*)(buffOut+4) = self_id;

	uint16_t varln = 6 - self_id % 7;
	*sizeInOut = 6+varln+1;
	buffOut[6+varln] = '>';
	while (varln--) buffOut[6+varln] = '-';

	// print outgoing packet
	char tail[9];
	memcpy( tail, buffOut+6, *sizeInOut - 6 );
	tail[ *sizeInOut - 6 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%04x][0x%04x]%s\n", first_byte, second_byte, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 7 && *sizeInOut <= 14 );

	// return status
	return YOCTOVM_PASS_LOWER;
}

//bool start_over_once = true;
bool start_over_once = false;
bool statck_reset = false;

uint8_t master_continue( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ )
{
	// by now master_continue() does the same as yocto_process


	// get packet data
	uint8_t first_byte = buffIn[0];
	uint8_t second_byte = buffIn[1];
	uint16_t reply_to_id = *(uint16_t*)(buffIn+2);
	uint16_t self_id = *(uint16_t*)(buffIn+4);
	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits

	if ( start_over_once )
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

	// print received packet
	char tail[9];
	memcpy( tail, buffIn+6, *sizeInOut - 6 );
	tail[ *sizeInOut - 6 ] = 0;
	PRINTF( "Yocto: Packet received: [%d][%d][0x%04x][0x%04x]%s\n", first_byte, second_byte, reply_to_id, self_id, tail );

	// check packet data
	assert( *sizeInOut >= 7 && *sizeInOut <= 14 );
	if ( first_byte == SAGDP_P_STATUS_FIRST )
		assert( 0 == reply_to_id );
	else
		assert( last_sent_id == reply_to_id );

	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
	{
//		return YOCTOVM_OK;
		return master_start( sizeInOut, buffIn, buffOut );
	}

	// fake implementation: should this packet be terminal?
	if ( second_byte <= 1 )
	{
		first_byte = SAGDP_P_STATUS_TERMINATING;
		chainContinued = false;
	}
	else
	{
		first_byte = SAGDP_P_STATUS_INTERMEDIATE;
		chainContinued = true;
	}
	second_byte--;


	// prepare outgoing packet
	reply_to_id = self_id;
	self_id++;
	last_sent_id = self_id;

	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	*(uint16_t*)(buffOut+2) = reply_to_id;
	*(uint16_t*)(buffOut+4) = self_id;

	uint16_t varln = 6 - self_id % 7;
	*sizeInOut = 6+varln+1;
	buffOut[6+varln] = '>';
	while (varln--) buffOut[6+varln] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+6, *sizeInOut - 6 );
	tail[ *sizeInOut - 6 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%04x][0x%04x]%s\n", first_byte, second_byte, reply_to_id, self_id, tail );

	assert( *sizeInOut >= 7 && *sizeInOut <= 14 );

	// return status
	return YOCTOVM_PASS_LOWER;
}
