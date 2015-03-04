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
// End of Pure Testing Block 

#if !defined USED_AS_MASTER

uint8_t yocto_process( uint16_t* sizeInOut, unsigned char* buffIn, unsigned char* buffOut/*, int buffOutSize, unsigned char* stack, int stackSize*/ )
{
	// buffer assumes to contain an input message (including First Byte)
	// FAKE structure of the buffer: First_Byte | remaining_packet_cnt (1 byte) | fake_body (9-12 bytes)
	// fake body is used for external controlling of data integrity and is organized as follows:
	// replying_to (4 bytes) | self_id (4 bytes) | rand_part (1-4 bytes)
	// self_id is a randomly generated 4-byte integer (for simplicity we assume that in the test environment comm peers have the same endiannes)
	// replying_to is a copy of self_id of the received packet (or 0 for the first packet ion the chain)
	// rand_part is filled with some number of '-' ended by '>'

	assert( *sizeInOut >= 11 && *sizeInOut <= 14 );

	uint8_t first_byte = buffIn[0];
	uint8_t second_byte = buffIn[1];

	// print received packet
	char tail[5];
	memcpy( tail, buffIn+10, *sizeInOut - 10 );
	tail[ *sizeInOut - 10 ] = 0;
	PRINTF( "Yocto: Packet received: [%d][%d][0x%x%x%x%x][0x%x%x%x%x]%s\n", first_byte, second_byte, buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6], buffIn[7], buffIn[8], buffIn[9], tail );

	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
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
	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	memcpy( buffOut+2, buffIn+6, 4);
	memcpy( buffOut+6, buffIn+6, 4);
	buffOut[9]++;
	uint16_t varln = buffOut[9] % 3;
	*sizeInOut = 10+varln+1;
	buffOut[10+varln] = '>';
	while (varln--) buffOut[10+varln] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+10, *sizeInOut - 10 );
	tail[ *sizeInOut - 10 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%x%x%x%x][0x%x%x%x%x]%s\n", first_byte, second_byte, buffOut[2], buffOut[3], buffOut[4], buffOut[5], buffOut[6], buffOut[7], buffOut[8], buffOut[9], tail );

	// return status
	chainContinued = true;
	return YOCTOVM_PASS_LOWER;
}

#else // USED_AS_MASTER

uint8_t master_start( uint16_t* sizeInOut, unsigned char* buffIn, unsigned char* buffOut/*, int buffOutSize, unsigned char* stack, int stackSize*/ )
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

	// prepare outgoing packet
	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	memset( buffOut+2, 0, 4);
	memset( buffOut+6, 0, 4);
	uint16_t varln = buffOut[9] % 3;
	*sizeInOut = 10+varln+1;
	buffOut[10+varln] = '>';
	while (varln--) buffOut[10+varln-1] = '-';

	// print outgoing packet
	char tail[5];
	memcpy( tail, buffOut+10, *sizeInOut - 10 );
	tail[ *sizeInOut - 10 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%x%x%x%x][0x%x%x%x%x]%s\n", first_byte, second_byte, buffOut[2], buffOut[3], buffOut[4], buffOut[5], buffOut[6], buffOut[7], buffOut[8], buffOut[9], tail );

	// return status
	return YOCTOVM_PASS_LOWER;
}

uint8_t master_continue( uint16_t* sizeInOut, unsigned char* buffIn, unsigned char* buffOut/*, int buffOutSize, unsigned char* stack, int stackSize*/ )
{
	// by now master_continue() does the same as yocto_process

	assert( *sizeInOut >= 11 && *sizeInOut <= 14 );

	uint8_t first_byte = buffIn[0];
	uint8_t second_byte = buffIn[1];

	// print received packet
	char tail[5];
	memcpy( tail, buffIn+10, *sizeInOut - 10 );
	tail[ *sizeInOut - 10 ] = 0;
	PRINTF( "Yocto: Packet received: [%d][%d][0x%x%x%x%x][0x%x%x%x%x]%s\n", first_byte, second_byte, buffIn[2], buffIn[3], buffIn[4], buffIn[5], buffIn[6], buffIn[7], buffIn[8], buffIn[9], tail );

	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
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
	buffOut[0] = first_byte;
	buffOut[1] = second_byte;
	memcpy( buffOut+2, buffIn+6, 4);
	memcpy( buffOut+6, buffIn+6, 4);
	buffOut[9]++;
	uint16_t varln = buffOut[9] % 3;
	*sizeInOut = 10+varln+1;
	buffOut[10+varln] = '>';
	while (varln--) buffOut[10+varln-1] = '-';

	// print outgoing packet
	memcpy( tail, buffOut+10, *sizeInOut - 10 );
	tail[ *sizeInOut - 10 ] = 0;
	PRINTF( "Yocto: Packet sent    : [%d][%d][0x%x%x%x%x][0x%x%x%x%x]%s\n", first_byte, second_byte, buffOut[2], buffOut[3], buffOut[4], buffOut[5], buffOut[6], buffOut[7], buffOut[8], buffOut[9], tail );

	// return status
	return YOCTOVM_PASS_LOWER;
}

#endif // USED_AS_MASTER