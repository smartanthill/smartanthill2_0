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

#if !defined __HAL_EEPROM_H__
#define __HAL_EEPROM_H__

#include "../common/sa_common.h"

#ifdef __cplusplus
extern "C" {
#endif

bool hal_init_eeprom_access();
bool hal_eeprom_write( const uint8_t* data, uint16_t size, uint16_t address );
bool hal_eeprom_read( uint8_t* data, uint16_t size, uint16_t address);
void hal_eeprom_flush();

#ifdef __cplusplus
}
#endif

#endif // __HAL_EEPROM_H__