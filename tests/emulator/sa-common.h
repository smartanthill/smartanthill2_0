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


#if !defined __SA_COMMON_H__
#define __SA_COMMON_H__

// common includes
#include <memory.h> // for memcpy(), memset(), memcmp(). Note: their implementation may or may not be more effective than just by-byte operation on a particular target platform
#include <assert.h>

// data types
#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short

// Master/Slave distinguishing bit; USED_AS_MASTER is assumed to be a preprocessor definition if necessary
#ifdef USED_AS_MASTER
#define MASTER_SLAVE_BIT 1
#else // USED_AS_MASTER
#define MASTER_SLAVE_BIT 0
#endif

// offsets in data segment of particular handler data
// note: internal structure is defined by a correspondent handler (see respective .h files for details)
// TODO: think about more reliable mechanism
#define DADA_OFFSET_SASP 0
#define DADA_OFFSET_SAGDP ( DADA_OFFSET_SASP + 28 )
#define DADA_OFFSET_NEXT ( DADA_OFFSET_SAGDP + 28 )

// debug helpers

#define DEBUG_PRINTING

#ifdef DEBUG_PRINTING
#include <stdio.h>
#define PRINTF printf
#else // DEBUG_PRINTING
#define PRINTF
#endif // DEBUG_PRINTING

#endif // __SA_COMMON_H__