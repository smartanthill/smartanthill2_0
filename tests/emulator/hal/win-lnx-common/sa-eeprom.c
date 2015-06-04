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

#include "../sa-eeprom.h"
#include <stdio.h> 


// Interface is implemented based on file IO
// Files have standard names, each corresponding to master/slave distinguisher and data ID

void prepareFileName( char* nameBuff, uint8_t id )
{
	sprintf( nameBuff, "eeprom-%s-%d.dat", MASTER_SLAVE_BIT ? "master" : "slave", id );
}

void eeprom_write( uint8_t id, uint8_t* data, uint8_t size)
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "wb" );
	assert( f );
	int written = fwrite( data, 1, size, f );
	assert( written == size );
	fclose( f );
}

void eeprom_read_fixed_size( uint8_t id, uint8_t* data, uint8_t size)
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "rb" );
	assert( f );
	int retrieved = fread ( data, 1, size, f );
	assert( retrieved == size );
	fclose( f );
}

uint8_t eeprom_read_size( uint8_t id )
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "wb" );
	assert( f );
	fseek ( f, 0, SEEK_END );
	int size = ftell( f );
	fclose( f );
	return size;
}
