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

#if !defined __SA_EEPROM_H__
#define __SA_EEPROM_H__

#include "sa-common.h"

// data IDs (for communication with eeprom
#define EEPROM_SLOT_DATA_SASP_NONCE_LW_ID 0 // Nonce Lower Watermark
#define EEPROM_SLOT_DATA_SASP_NONCE_LS_ID 1 // Nonce to use For Sending

#define EEPROM_SLOT_MAX 2
// ...to be continued

#define DATA_CONTINUE_LIFE_ID 0Xff // FAKE data used at simulator startup: if not present, a new life (whatever it means) is started


// calls
bool init_eeprom_access();
void eeprom_write( uint8_t id, uint8_t* data);
void eeprom_read( uint8_t id, uint8_t* data);

#endif // __SA_EEPROM_H__