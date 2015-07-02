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


#include "../../firmware/src/common/sa_common.h"

#ifdef ENABLE_COUNTER_SYSTEM
size_t COUNTERS[MAX_COUNTERS_CNT];
const char* CTRS_NAMES[MAX_COUNTERS_CNT];
double COUNTERS_D[MAX_COUNTERS_CNT];
const char* CTRS_NAMES_D[MAX_COUNTERS_CNT];
void printCounters()
{
	ZEPTO_DEBUG_PRINTF_1( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n" );
	uint16_t i;
	for ( i=0; i<MAX_COUNTERS_CNT; i++ )
	{
		if ( CTRS_NAMES[ i ] )
			ZEPTO_DEBUG_PRINTF_4( "%d:\t[%d] %s\n", COUNTERS[ i ], i, CTRS_NAMES[ i ] );
		else
			ZEPTO_DEBUG_ASSERT( COUNTERS[ i ] == 0 );
		if ( CTRS_NAMES_D[ i ] )
			ZEPTO_DEBUG_PRINTF_4( "%f:\t[%d] %s\n", COUNTERS_D[ i ], i, CTRS_NAMES_D[ i ] );
	}
	ZEPTO_DEBUG_PRINTF_1( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n" );
}
#endif // ENABLE_COUNTER_SYSTEM

/*

struct counter
{
	const char* name;
	int value;
};

counter Counters[] =
{
};


void increment_counter( uint8_t ctr )
{
}

void print_counters()
{
}
*/