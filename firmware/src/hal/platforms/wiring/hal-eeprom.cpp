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

#include "../../hal-eeprom.h"
#include <EEPROM.h>

#define EEPROM_SIZE (EEPROM.length())


static inline bool _validate_operation (uint16_t size, uint16_t address)
{
    if ((address+size > EEPROM_SIZE) || (size > EEPROM_SIZE) || (address > EEPROM_SIZE))
        return false;
    else
        return true;
}


bool hal_init_eeprom_access()
{
	return true;
}

bool hal_eeprom_write( const uint8_t* data, uint16_t size, uint16_t address )
{
	if (_validate_operation (size, address)) {
        for (uint32_t i = 0; i < size; i++) {
            EEPROM.write(address++, data[i]);
        }
        return true;
    }

    return false;
}

bool hal_eeprom_read( uint8_t* data, uint16_t size, uint16_t address)
{
    if (_validate_operation (size, address)) {
        for (uint32_t i = 0; i < size; i++) {
            data[i] = EEPROM.read(address++);
        }
        return true;
    }

    return false;
}

void hal_eeprom_flush()
{
	for (uint32_t i = 0; i < EEPROM.length(); i++ ) {
         EEPROM.write(i, 0);
    }
}

#endif
