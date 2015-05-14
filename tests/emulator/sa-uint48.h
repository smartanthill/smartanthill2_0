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

#if ! defined __SA_UINT48_H__
#define __SA_UINT48_H__

#include "sa-common.h"
#include "zepto-mem-mngmt.h"

typedef uint8_t sa_uint48_t[6];

inline
void sa_uint48_set_zero( sa_uint48_t t )
{
	memset( t, 0, 6 );
}

inline
void sa_uint48_init_by( sa_uint48_t t, const sa_uint48_t t_src )
{
	memcpy( t, t_src, 6 );
}

inline
void zepto_parser_encode_and_append_sa_uint48( MEMORY_HANDLE mem_h, const sa_uint48_t t )
{
	zepto_parser_encode_and_append_uint( mem_h, t, 6 );
}

inline
void zepto_parser_encode_and_prepend_sa_uint48( MEMORY_HANDLE mem_h, const sa_uint48_t t )
{
	zepto_parser_encode_and_prepend_uint( mem_h, t, 6 );
}

inline
void zepto_parser_decode_encoded_uint_as_sa_uint48( parser_obj* po, sa_uint48_t t )
{
	zepto_parser_decode_uint( po, t, 6 );
}

#if (SA_USED_ENDIANNES == SA_LITTLE_ENDIAN)

inline
void sa_uint48_increment(  sa_uint48_t t )
{
	int8_t i;
	for ( i=0; i<6; i++ )
	{
		t[i] ++;
		if ( t[i] ) break;
	}
}

inline
int8_t sa_uint48_compare( const sa_uint48_t t1, const sa_uint48_t t2 )
{
	int8_t i;
	for ( i=5; i>=0; i-- )
	{
		if ( t1[i] > t2[i] ) return int8_t(1);
		if ( t1[i] < t2[i] ) return int8_t(-1);
	}
	return 0;
}

inline
bool is_uint48_zero( const sa_uint48_t t )
{
	return t[0] == 0 && t[1] == 0 && t[2] == 0 && t[3] == 0 && t[4] == 0 && t[5] == 0;
}

inline
uint8_t sa_uint48_get_byte( const sa_uint48_t t, uint8_t byte_num ) // returns a byte at position byte_num (in the range 0-5) so that 0 corresponds to the least significant byte
{
	assert( byte_num <= 5 );
	return t[ byte_num ];
}

#elif (SA_USED_ENDIANNES == SA_BIG_ENDIAN)

inline
void sa_uint48_increment(  sa_uint48_t t )
{
	int8_t i;
	for ( i=5; i>=0; i-- )
	{
		t[i] ++;
		if ( t[i] ) break;
	}
}

inline
int8_t sa_uint48_compare( const sa_uint48_t t1, const sa_uint48_t t2 )
{
	int8_t i;
	for ( i=0; i<6; i++ )
	{
		if ( t1[i] > t2[i] ) return int8_t(1);
		if ( t1[i] < t2[i] ) return int8_t(-1);
	}
	return 0;
}

inline
uint8_t sa_uint48_get_byte( const sa_uint48_t t, uint8_t byte_num ) // returns a byte at position byte_num (in the range 0-5) so that 0 corresponds to the least significant byte
{
	assert( byte_num <= 5 );
	return t[ 5 - byte_num ];
}

#else
#error SA_USED_ENDIANNES has unexpected value
#endif

#endif // __SA_UINT48_H__
