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

#if !defined __SASP_PROTOCOL_H__
#define __SASP_PROTOCOL_H__

#include "sa-common.h"
#include "sa-eeprom.h"

// RET codes
#define SASP_RET_IGNORE 0 // not authenticated, etc
#define SASP_RET_TO_HIGHER_NEW 1 // new packet
//#define SASP_RET_TO_HIGHER_REPEATED 2 // repeated packet
#define SASP_RET_TO_HIGHER_LAST_SEND_FAILED 3 // sending of last packet failed (for instance, old nonce)
#define SASP_RET_TO_LOWER_REGULAR 4 // for regular sending
#define SASP_RET_TO_LOWER_ERROR 5 // for error messaging
#define SASP_RET_NONCE 6 // buffer out contains nonce


// sizes
#define SASP_NONCE_SIZE 6
#define SASP_HEADER_SIZE SASP_NONCE_SIZE
#define SASP_ENC_BLOCK_SIZE 16
#define SASP_TAG_SIZE SASP_ENC_BLOCK_SIZE


// data structures
#define DATA_SASP_SIZE (SASP_NONCE_SIZE+SASP_NONCE_SIZE+SASP_TAG_SIZE)
#define DATA_SASP_NONCE_LW_OFFSET 0 // Nonce Lower Watermark
#define DATA_SASP_NONCE_LS_OFFSET SASP_NONCE_SIZE // Nonce to use For Sending
#define DATA_SASP_LRPS_OFFSET (SASP_NONCE_SIZE+SASP_NONCE_SIZE) // Last Received Packet Signature


// handlers
uint8_t handlerSASP_send( const uint8_t* nonce, uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSASP_receive( uint8_t* pid, uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data );
uint8_t handlerSASP_get_nonce( uint16_t* sizeInOut, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data );

#endif // __SASP_PROTOCOL_H__