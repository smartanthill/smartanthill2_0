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

#include "../hal-eeprom.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h> 
#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#endif


// Interface is implemented based on file IO
// File has a standard name

//FILE* f = NULL;
int efile = -1;
#ifdef _MSC_VER
HANDLE hfile = INVALID_HANDLE_VALUE;
#endif

bool hal_init_eeprom_access()
{
//	f = fopen( MASTER_SLAVE_BIT == 1 ? "sa-eeprom-master": "sa-eeprom-slave", "rw+b" );
	efile = open( MASTER_SLAVE_BIT == 1 ? "sa-eeprom-master": "sa-eeprom-slave.dat", O_RDWR | O_CREAT | O_BINARY, S_IWRITE | S_IREAD );
#ifdef _MSC_VER
	hfile = (HANDLE) _get_osfhandle (efile);
	if ( hfile == INVALID_HANDLE_VALUE )
		return false;
#endif
	return efile != -1;
}

bool hal_eeprom_write( const uint8_t* data, uint16_t size, uint16_t address )
{
	int res;
	res = lseek( efile, address, SEEK_SET);
	if ( res == -1 )
		return false;
	res = write( efile, data, size );
	if ( res !=  size )
		return false;
	return true;
}

bool hal_eeprom_read( uint8_t* data, uint16_t size, uint16_t address)
{
	int res;
	res = lseek( efile, address, SEEK_SET);
	if ( res == -1 )
		return false;
	res = read( efile, data, size );
	if ( res !=  size )
		return false;
	return true;
}

void hal_eeprom_flush()
{
#ifdef _MSC_VER
	FlushFileBuffers(hfile);
#else
	fsync( efile );
#endif
}

/*
void eeprom_write( uint8_t id, uint8_t* data, uint8_t size)
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "wb" );
	ZEPTO_DEBUG_ASSERT( f );
	int written = fwrite( data, 1, size, f );
	ZEPTO_DEBUG_ASSERT( written == size );
	fclose( f );
}

void eeprom_read_fixed_size( uint8_t id, uint8_t* data, uint8_t size)
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "rb" );
	ZEPTO_DEBUG_ASSERT( f );
	int retrieved = fread ( data, 1, size, f );
	ZEPTO_DEBUG_ASSERT( retrieved == size );
	fclose( f );
}

uint8_t eeprom_read_size( uint8_t id )
{
	char filename[256];
	prepareFileName( filename, id );
	FILE* f = fopen( filename, "wb" );
	ZEPTO_DEBUG_ASSERT( f );
	fseek ( f, 0, SEEK_END );
	int size = ftell( f );
	fclose( f );
	return size;
}
*/