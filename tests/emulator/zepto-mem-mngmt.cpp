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

#include "zepto-mem-mngmt.h"

struct request_reply_mem_obj
{ 
	uint8_t* ptr;
	uint16_t rq_size;
	uint16_t rsp_size;
};

#define REQUEST_REPLY_OBJ_MAX_CNT 128
request_reply_mem_obj memory_objects[ REQUEST_REPLY_OBJ_MAX_CNT ]; // fixed size array for a while

uint8_t* get_request_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].ptr;
}

uint16_t get_request_size( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].rq_size;
}

uint8_t* get_response_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size;
}

uint8_t* zepto_append( REQUEST_REPLY_HANDLE mem_h, uint16_t size )
{
	// TODO: reallocation logic
	uint8_t* ret = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size += size;
	return ret;
}


////	parsing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_parser_init( parser_obj* po, REQUEST_REPLY_HANDLE mem_h )
{
	po->mem_handle = mem_h;
	po->offset = 0;
}

uint8_t zepto_parse_uint8( parser_obj* po )
{
	uint8_t* buff = get_request_ptr( po->mem_handle );
	assert( buff != NULL );
	uint8_t ret = buff[ po->offset ];
	( po->offset ) ++;
	return ret;
}

uint16_t zepto_parse_encoded_uint16( parser_obj* po )
{
	uint8_t* buff = get_request_ptr( po->mem_handle );
	assert( buff != NULL );
	uint16_t ret;
	if ( ( buff[ 0 ] & 128 ) == 0 )
	{
		ret = (uint16_t)(buff[ 0 ]); 
		(po->offset)++;
		return ret;
	}
	else if (  ( buff[ 1 ] & 128 ) == 0  )
	{
		ret = 128 + ( (uint16_t)(buff[0] & 0x7F) | ( (uint16_t)(buff[1]) << 7) );
		po->offset += 2;
		return ret;
	}
	else
	{
		assert( (buff[2] & 0x80) == 0 );
		assert( buff[2] < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (uint16_t)(buff[1]) << 7) ) | ( (uint16_t)(buff[2]) << 14);
		po->offset += 3;
		return ret;
	}
}

void zepto_parse_read_block( parser_obj* po, uint8_t* block, uint16_t size )
{
	uint8_t* buff = get_request_ptr( po->mem_handle );
	assert( buff != NULL );
	assert( po->offset + size <= get_request_size( po->mem_handle ) );
	memcpy( block, buff, size );
	(po->offset) += size;
}

////	writing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_write_uint8( REQUEST_REPLY_HANDLE mem_h, uint8_t val )
{
	uint8_t* buff = zepto_append( mem_h, 1 );
	assert( buff != NULL );
	buff[0] = val;
}

void zepto_write_encoded_uint16( REQUEST_REPLY_HANDLE mem_h, uint16_t num )
{
	uint8_t* buff;
	assert( buff != NULL );
	if ( num < 128 )
	{
		buff = zepto_append( mem_h, 1 );
		*buff = num;
	}
	else if ( num < 16512 )
	{
		buff = zepto_append( mem_h, 2 );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (num - 128) >> 7;
	}
	else
	{
		buff = zepto_append( mem_h, 3 );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (num - 128) >> 7 + 128;
		buff[2] = (num - 128) >> 14;
	}
}

void zepto_write_block( REQUEST_REPLY_HANDLE mem_h, const uint8_t* block, uint16_t size )
{
	uint8_t* buff = zepto_append( mem_h, 3 );
	memcpy( buff, block, size );
}
