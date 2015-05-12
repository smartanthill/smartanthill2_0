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

#if !defined __SA_HAL_TIME_PROVIDER_H__
#define __SA_HAL_TIME_PROVIDER_H__

#include "sa-common.h"

struct _sa_time_struct
{
	uint16_t low_t;
	uint16_t high_t;
};
// NOTE: the struct above is not to be used directly
// use typedef below instead
typedef struct _sa_time_struct sa_time_val;

void sa_get_time( sa_time_val* t );


// operations (to be added upon necessity)

inline void sa_hal_time_val_copy_from( sa_time_val* t1, const sa_time_val* t2 )
{
	t1->high_t = t2->high_t;
	t1->low_t = t2->low_t;
}

inline bool sa_hal_time_val_is_less( sa_time_val* t1, sa_time_val* t2 )
{
	if ( t1->high_t < t2->high_t ) return true;
	if ( t1->high_t > t2->high_t ) return false;
	return t1->low_t < t2->low_t;
}

#define SA_TIME_LOAD_TICKS_FOR_1_SEC( x ) {(x).low_t = 1000; (x).high_t = 0;}
#define SA_TIME_INCREMENT_BY_TICKS( x, y ) {(x).low_t += (y).low_t; (x).high_t += (y).high_t; (x).high_t += (x).low_t < (y).low_t ? 1 : 0;}
#define SA_TIME_SUBTRACT_TICKS_OR_ZERO( x, y ) {if ( sa_hal_time_val_is_less( &(x), (y) ) ) {(x).low_t = 0; (x).high_t = 0;} else {(x).low_t -= (y).low_t; (x).high_t -= (y).high_t; (x).high_t -= (x).low_t < (y).low_t ? 1 : 0;} }
#define SA_TIME_MUL_TICKS_BY_2( x ) {uint16_t tmp = ((x).low_t) >> 15; (x).low_t <<= 1; (x).high_t = ((x).high_t << 1) | tmp;}
#define SA_TIME_MUL_TICKS_BY_1_AND_A_HALF( x ) {uint16_t lo = (x).low_t, hi = (x).high_t; uint16_t tmp = ((x).high_t) & 1; (x).low_t >>= 15; (x).low_t |= tmp << 15; (x).high_t = ((x).high_t << 1) | tmp; (x).low_t += lo; (x).high_t += hi; (x).high_t += (x).low_t < lo ? 1 : 0;}

#endif // __SA_HAL_TIME_PROVIDER_H__