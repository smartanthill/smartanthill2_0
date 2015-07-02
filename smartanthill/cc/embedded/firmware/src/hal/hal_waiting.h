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

#if !defined __SA_HAL_WAITING_H__
#define __SA_HAL_WAITING_H__

#include "../common/sa_common.h"
#include "hal_time_provider.h"

typedef struct _waiting_for
{
	sa_time_val wait_time;
	uint8_t wait_packet;
	uint8_t wait_i2c;
	uint8_t wait_legs;
	uint16_t leg_mask; // [in]
	uint16_t leg_values_waited; // [in]
	uint16_t leg_value_received; // [out]
} waiting_for;

#define WAIT_RESULTED_IN_FAILURE 0
#define WAIT_RESULTED_IN_TIMEOUT 1
#define WAIT_RESULTED_IN_PACKET 2
#define WAIT_RESULTED_IN_I2C 3
#define WAIT_RESULTED_IN_PINS 4

#ifdef __cplusplus
extern "C" {
#endif

uint8_t hal_wait_for( waiting_for* wf );
uint8_t wait_for_timeout( uint32_t timeout );
void mcu_sleep( uint16_t sec, uint8_t transmitter_state_on_exit );
void just_sleep( sa_time_val* timeval );
void keep_transmitter_on( bool keep_on );

#ifdef __cplusplus
}
#endif

#endif // __SA_HAL_WAITING_H__
