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


#if !defined __YOCTOVM_PROTOCOL_H__
#define __YOCTOVM_PROTOCOL_H__

#include "sa-common.h"
#include "sa-eeprom.h"

// RET codes
#define YOCTOVM_FAILED 0 // sunject for system reset
#define YOCTOVM_OK 1 // if terminating packet received
#define YOCTOVM_PASS_LOWER 2
#define YOCTOVM_RESET_STACK 3

uint8_t slave_process( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
uint8_t master_start( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );
uint8_t master_continue( uint16_t* sizeInOut, const uint8_t* buffIn, uint8_t* buffOut/*, int buffOutSize, uint8_t* stack, int stackSize*/ );

// Pure Testing Block
bool isChainContinued();
// End of Pure Testing Block 

#endif // __YOCTOVM_PROTOCOL_H__