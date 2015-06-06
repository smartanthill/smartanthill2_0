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

#ifdef WINLNXCOMMON

#include "../../hal-eeprom.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
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

#endif

