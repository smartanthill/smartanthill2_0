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

#if defined ARDUINO && (!defined ENERGIA)

#if !defined __HAL_PLATFORM_ARDUINO_MAIN_H__
#define __HAL_PLATFORM_ARDUINO_MAIN_H__

#include <avr/pgmspace.h>
#include <Arduino.h>
#include "stdint.h"
#include "hal-time-conversions.h"

#define ZEPTO_PROGMEM_IN_USE
#define ZEPTO_PROGMEM      __attribute__ ((progmem))
#define ZEPTO_PROG_CONSTANT_LOCATION ZEPTO_PROGMEM
#define ZEPTO_PROG_CONSTANT_READ_BYTE(x) pgm_read_byte(x)
#define ZEPTO_MEMCPY_FROM_PROGMEM memcpy_PF

#endif // __HAL_PLATFORM_ARDUINO_MAIN_H__

#endif // ARDUINO && (!defined ENERGIA)
