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

#include "sa-eeprom.h"
#include "hal/hal-eeprom.h"

// slot structure:
// [byte 0]   record selection bit
// [byte 1-2] record size
// [...]      record 1 old/new
// [...]      record 2 old/new
// latest record is pointed based on the value of record selection bit (0: first; 1: second)

#define SLOT_SIZE_FROM_RECORD_SIZE( x ) ( 2 * (x) + 3 )


// data offsets and sizes
#define DATA_SASP_NONCE_LW_SIZE 6 // Nonce Lower Watermark
#define DATA_SASP_NONCE_LS_SIZE 6 // Nonce to use For Sending

typedef struct _eeprom_slot_descriptor
{
	uint16_t offset;
	uint16_t size;
} eeprom_slot_descriptor;

eeprom_slot_descriptor eeprom_slots[] =
{
	{0, DATA_SASP_NONCE_LW_SIZE},
	{SLOT_SIZE_FROM_RECORD_SIZE( DATA_SASP_NONCE_LW_SIZE ), DATA_SASP_NONCE_LS_SIZE },
};

bool init_eeprom_access()
{
	return hal_init_eeprom_access();
}

void format_eeprom_at_lifestart()
{
	uint8_t buff[32];
	uint8_t i, j;
	for ( i=0; i<EEPROM_SLOT_MAX; i++ )
	{
		eeprom_slot_descriptor* descr = eeprom_slots + i;
		buff[0] = 0;
		buff[1] = descr->size;
		buff[2] = descr->size >> 8;
		hal_eeprom_write( buff, 3, descr->offset );
	}
	return true;
}

void eeprom_write( uint8_t id, uint8_t* data)
{
	ZEPTO_DEBUG_ASSERT( id < EEPROM_SLOT_MAX );
	uint8_t buff[3];
	memset( buff, 0, 3 );
	bool res;
	res = hal_eeprom_read( buff, 3, eeprom_slots[id].offset );
	ZEPTO_DEBUG_ASSERT( res );
	uint16_t sz = ((uint16_t)(buff[2]) << 8) + buff[1];
	ZEPTO_DEBUG_ASSERT( sz == eeprom_slots[id].size ); // TODO: do we need both?
	if ( buff[0] == 1 )
	{
		hal_eeprom_write( data, sz, eeprom_slots[id].offset + 3 );
		hal_eeprom_flush();
		buff[0] = 0;
		hal_eeprom_write( buff, 1, eeprom_slots[id].offset );
		hal_eeprom_flush();
	}
	else
	{
		hal_eeprom_write( data, sz, eeprom_slots[id].offset + sz + 3 );
		hal_eeprom_flush();
		buff[0] = 1;
		hal_eeprom_write( buff, 1, eeprom_slots[id].offset );
		hal_eeprom_flush();
	}
}

void eeprom_read( uint8_t id, uint8_t* data)
{
	ZEPTO_DEBUG_ASSERT( id < EEPROM_SLOT_MAX );
	uint8_t buff[3];
	hal_eeprom_read( buff, 3, eeprom_slots[id].offset );
	uint16_t sz = ((uint16_t)(buff[2]) << 8) + buff[1];
	ZEPTO_DEBUG_ASSERT( sz == eeprom_slots[id].size ); // TODO: do we need both?
	if ( buff[0] == 1 )
		hal_eeprom_read( data, sz, eeprom_slots[id].offset + 3 + sz );
	else
		hal_eeprom_read( data, sz, eeprom_slots[id].offset + 3 );
}
