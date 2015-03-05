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


#if !defined __YOCTOVM_PROTOCOL_H__
#define __YOCTOVM_PROTOCOL_H__

#include "sa-common.h"
#include "sa-eeprom.h"

// RET codes
#define YOCTOVM_FAILED 0 // sunject for system reset
#define YOCTOVM_OK 1 // if terminating packet received
#define YOCTOVM_PASS_LOWER 2

#if !defined USED_AS_MASTER
uint8_t yocto_process( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
#else // USED_AS_MASTER
uint8_t master_start( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
uint8_t master_continue( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
#endif // USED_AS_MASTER

// Pure Testing Block
bool isChainContinued();
// End of Pure Testing Block 

#endif // __YOCTOVM_PROTOCOL_H__