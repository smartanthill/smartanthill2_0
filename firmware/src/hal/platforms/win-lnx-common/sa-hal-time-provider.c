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

#ifdef WINLNXCOMMON

#include "../../sa-hal-time-provider.h"


#ifdef _MSC_VER

#include <Windows.h>

void sa_get_time( sa_time_val* t )
{
	unsigned int sys_t = GetTickCount();
	t->high_t = sys_t >> 16;
	t->low_t = (unsigned short)sys_t;
}

unsigned short getTime()
{
	return (unsigned short)( GetTickCount() / 200 );
}

#else

#include <unistd.h>
#include <time.h>

#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_MONOTONIC 1

int clock_gettime(int clk_id, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

uint32_t getTick() {
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}


void sa_get_time( sa_time_val* t )
{
	unsigned int sys_t = getTick();
	t->high_t = sys_t >> 16;
	t->low_t = (unsigned short)sys_t;
}

// TODO: get rid of it
unsigned short getTime()
{
	return (unsigned short)( getTick() / 200 );
}

#endif

#endif
