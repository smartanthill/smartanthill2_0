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
#include <string.h> // for memmove()
#include <assert.h>

#define SA_DEBUG

#define SA_LITTLE_ENDIAN 0
#define SA_BIG_ENDIAN 1
#define SA_USED_ENDIANNES SA_LITTLE_ENDIAN
//#define SA_USED_ENDIANNES SA_BIG_ENDIAN

// data types
#ifdef _MSC_VER
#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short
#define int16_t short
#else
#include "stdint.h"
#endif
#define bool uint8_t
#define true 1
#define false 0

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE static inline
#endif



/*
INLINE void memset( void* dest, uint8_t val, uint8_t cnt )
{
	uint8_t i;
	for ( i=0; i<cnt; i++ )
		((uint8_t*)dest)[i] = val;
}

INLINE void memcpy( void* dest, const void* src, uint8_t cnt )
{
	uint8_t i;
	for ( i=0; i<cnt; i++ )
		((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
}
*/
// Master/Slave distinguishing bit; USED_AS_MASTER is assumed to be a preprocessor definition if necessary
#ifdef USED_AS_MASTER
#define MASTER_SLAVE_BIT 1
#else // USED_AS_MASTER
#define MASTER_SLAVE_BIT 0
#endif

// debug helpers

#define DEBUG_PRINTING

#ifdef DEBUG_PRINTING
#include <stdio.h>
#define PRINTF printf
#else // DEBUG_PRINTING
#define PRINTF
#endif // DEBUG_PRINTING


// counter system
#define ENABLE_COUNTER_SYSTEM

#define ENABLE_COUNTER_SYSTEM
#ifdef ENABLE_COUNTER_SYSTEM
#define MAX_COUNTERS_CNT 100
extern size_t COUNTERS[MAX_COUNTERS_CNT];
extern const char* CTRS_NAMES[MAX_COUNTERS_CNT];
extern double COUNTERS_D[MAX_COUNTERS_CNT];
extern const char* CTRS_NAMES_D[MAX_COUNTERS_CNT];
#define INIT_COUNTER_SYSTEM \
	memset( COUNTERS, 0, sizeof(COUNTERS) ); \
	memset( CTRS_NAMES, 0, sizeof(CTRS_NAMES) ); \
	memset( COUNTERS_D, 0, sizeof(COUNTERS_D) ); \
	memset( CTRS_NAMES_D, 0, sizeof(CTRS_NAMES_D) ); 
void printCounters();
#define PRINT_COUNTERS() printCounters()
#define TEST_CTR_SYSTEM
#ifdef TEST_CTR_SYSTEM
#define INCREMENT_COUNTER( i, name ) \
	{if ( (COUNTERS[i]) != 0 ); else { assert( CTRS_NAMES[i] == NULL ); CTRS_NAMES[i] = name; } \
	(COUNTERS[i])++;}
#define INCREMENT_COUNTER_IF( i, name, cond ) \
	{ if ( cond ) {if ( (COUNTERS[i]) != 0 ); else { assert( CTRS_NAMES[i] == NULL ); CTRS_NAMES[i] = name; } \
	(COUNTERS[i])++;} }
#define UPDATE_MAX_COUNTER( i, name, val ) \
	{CTRS_NAMES[i] = name; \
	if ( COUNTERS[i] >= val ); else COUNTERS[i] = val;}
#else // TEST_CTR_SYSTEM
#define INCREMENT_COUNTER( i, name ) (COUNTERS[i])++;
#endif // TEST_CTR_SYSTEM

#else
#define PRINT_COUNTERS()
#endif // ENABLE_COUNTER_SYSTEM

#endif // __SA_COMMON_H__
