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

uint8_t yocto_process( uint16_t* sizeInOut, unsigned char* buffIn, unsigned char* buffOut/*, int buffOutSize, unsigned char* stack, int stackSize*/ )
{
	// buffer assumes to contain an input message; interface is subject to change
	// by now this fanction has totally fake implementation; we need something to work with
	// anyway, in a real case there will be some kind of message source, for instance, a controlling application at Master side
	uint8_t first_byte = buffOut[0];
	first_byte &= SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING; // to use only respective bits
	if ( first_byte == SAGDP_P_STATUS_TERMINATING )
		return YOCTOVM_OK;

	// fake implementation: should this packet be terminal?
	uint8_t second_byte = buffOut[1];
	if ( second_byte )
	{
		second_byte--;
		first_byte = SAGDP_P_STATUS_INTERMEDIATE;
	}
	else
		first_byte = SAGDP_P_STATUS_TERMINATING;

	// fake implementation: prepare "visual part" of the packet

	uint16_t msgSize = *sizeInOut - 2;
	unsigned char flags = buffIn[0];
	unsigned char* payload_buff = buffIn + 1;
	PRINTF("Preparing reply to client message: [0x%02x]\"%s\" [1+%d bytes]\n", flags, payload_buff, msgSize-1 );
	sprintf( (char*)buffOut + 1, "Server reply; client message: [0x%02x]\"%s\" [1+%d bytes]", flags, payload_buff, msgSize-1 );
	buffOut[0] = buffIn[0]; // just echo so far
	int size = 0;
	while ( (buffOut + 1)[size++] );
	PRINTF("Reply is about to be sent: \"%s\" [%d bytes]\n", buffOut + 1, size);
	*sizeInOut = size+1;
	return 1;
}
