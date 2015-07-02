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

#include "../../hal_time_provider.h"
#include "../../hal_waiting.h"

void sa_get_time(sa_time_val* t)
{
    uint32_t sys_t = millis();
    t->high_t = sys_t >> 16;
    t->low_t = (uint16_t)sys_t;
}

uint32_t getTime()
{
    return millis();
}

void mcu_sleep( uint16_t sec, uint8_t transmitter_state_on_exit )
{
    if ( transmitter_state_on_exit == 0 )
        keep_transmitter_on( false );
    delay( ( uint16_t )sec * 1000 );
    if ( transmitter_state_on_exit )
        keep_transmitter_on( true );
}

void just_sleep( sa_time_val* timeval )
{
    uint32_t timeout = timeval->high_t;
    timeout <<= 16;
    timeout += timeval->low_t;
    wait_for_timeout( timeout);
    // TODO: add implementation
}

void keep_transmitter_on( bool keep_on )
{
    // TODO: add reasonable implementation
}
