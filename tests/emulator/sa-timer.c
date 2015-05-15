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

#include "sa-timer.h"


#ifdef _MSC_VER

#include <Windows.h>

#define TIME_FACTOR 200 // resulting in 200 ms granularity

void waitForTimeQuantum()
{
	Sleep(TIME_FACTOR);
}
/*
unsigned short getTime()
{
	return (unsigned short)( GetTickCount() / TIME_FACTOR );
}
*/
#endif