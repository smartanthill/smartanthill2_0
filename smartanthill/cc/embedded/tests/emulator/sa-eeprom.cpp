/*******************************************************************************
    Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source and compiled
    forms, with or without modification, are permitted
    provided that the following conditions are met:
        * Redistributions in source form must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in compiled form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the OLogN Technologies AG nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE
*******************************************************************************/

#include "sa-common.h"
#include "sa-eeprom.h"
#include <stdio.h> 


// Interface is implemented based on file IO
// Files have standard names, each corresponding to master/slave distinguisher and data ID

void prepareFileName( char* nameBuff, unsigned char id )
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
