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

#ifdef ENERGIA

#include "../../sa-commlayer.h"
#include "../../hal-waiting.h"

uint8_t hal_wait_for( waiting_for* wf )
{
	ZEPTO_DEBUG_ASSERT(0);
	return 0;
}

uint8_t tryGetMessage( MEMORY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT(0);
	return 0;
}

uint8_t communication_initialize()
{
	ZEPTO_DEBUG_ASSERT(0);
	return false;
}

uint8_t sendMessage( MEMORY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT(0);
	return 0;
}

#endif
