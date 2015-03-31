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
/*
struct request_reply_mem_obj
{ 
	uint8_t* ptr;
	uint16_t rq_size;
	uint16_t rsp_size;
};
*/
#define REQUEST_REPLY_OBJ_MAX_CNT 128
request_reply_mem_obj memory_objects[ REQUEST_REPLY_OBJ_MAX_CNT ]; // fixed size array for a while

uint8_t* memory_object_get_request_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].ptr;
}

uint16_t memory_object_get_request_size( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].rq_size;
}

uint8_t* memory_object_get_response_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size;
}

uint16_t memory_object_get_response_size( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < REQUEST_REPLY_OBJ_MAX_CNT );
	return memory_objects[ mem_h ].rsp_size;
}

uint8_t* memory_object_append( REQUEST_REPLY_HANDLE mem_h, uint16_t size )
{
printf( "memory_object_append( %d )\n", size ); 
	// TODO: reallocation logic
	uint8_t* ret = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size += size;
	return ret;
}

void memory_object_cut_and_make_response( REQUEST_REPLY_HANDLE mem_h, uint16_t offset, uint16_t size )
{
	assert( offset <= memory_objects[ mem_h ].rq_size );
	assert( offset + size <= memory_objects[ mem_h ].rq_size );
	memory_objects[ mem_h ].ptr += offset;
	// TODO: the above line may require further actions for "returning" memory
	memory_objects[ mem_h ].rq_size = 0;
	memory_objects[ mem_h ].rsp_size = size;
	// TODO: the above line may require further actions for "returning" memory
}

void memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, const uint8_t* buff, uint16_t size )
{
	// TODO: we have a few options here w/resp to request: to kill it, or to ensure that there is no request, or to move responce accordingly
	//	assert( 0 == memory_objects[ mem_h ].rq_size );
	memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
	memory_objects[ mem_h ].rq_size = 0;



	memory_objects[ mem_h ].ptr -= size;
	// TODO: the above line may require further actions for "getting" memory
	memory_objects[ mem_h ].rsp_size += size;
	memcpy( memory_objects[ mem_h ].ptr, buff, size );
}
/*
void memory_object_append( REQUEST_REPLY_HANDLE mem_h, uint8_t* buff, uint16_t size )
{
	assert( 0 == memory_objects[ mem_h ].rq_size );
	// TODO: the below line may require further actions for "getting" memory
	memcpy( memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rsp_size, buff, size );
	memory_objects[ mem_h ].rsp_size += size;
}
*/
void memory_object_response_to_request( REQUEST_REPLY_HANDLE mem_h )
{
	// TODO: the below lines may require further actions for "releasing" memory
	memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
	memory_objects[ mem_h ].rq_size = memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size = 0;
}


////	parsing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_parser_init( parser_obj* po, REQUEST_REPLY_HANDLE mem_h )
{
	po->mem_handle = mem_h;
	po->offset = 0;
}

void zepto_parser_init( parser_obj* po, const parser_obj* po_base )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	po->mem_handle = po_base->mem_handle;
	po->offset = po_base->offset;
}

uint8_t zepto_parse_uint8( parser_obj* po )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	assert( buff != NULL );
	uint8_t ret = buff[ 0 ];
	( po->offset ) ++;
	return ret;
}

uint16_t zepto_parse_encoded_uint16( parser_obj* po )
{
printf( "zepto_write_encoded_uint16(): ini offset = %d...", po->offset ); 
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	assert( buff != NULL );
	uint16_t ret;
	if ( ( buff[ 0 ] & 128 ) == 0 )
	{
		ret = (uint16_t)(buff[ 0 ]); 
		(po->offset)++;
	}
	else if (  ( buff[ 1 ] & 128 ) == 0  )
	{
		ret = 128 + ( (uint16_t)(buff[0] & 0x7F) | ( ((uint16_t)(buff[1])) << 7) ); 
		po->offset += 2;
	}
/*	else if (buff[0] == 0x80 && buff[1] == 0xff && buff[2] == 1 )
	{
		ret = 0x8000;
		po->offset += 3;
	}*/
	else
	{
		assert( (buff[2] & 0x80) == 0 );
		assert( buff[2] < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (uint16_t)(buff[1]) << 7) ) | ( ((uint16_t)(buff[2])) << 14);
		po->offset += 3;
	}
	printf( "new offset = %d, num = %x\n", po->offset, ret ); 
	return ret;
}

bool zepto_parse_read_block( parser_obj* po, uint8_t* block, uint16_t size )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	if ( po->offset + size > memory_object_get_request_size( po->mem_handle ) )
		return false;
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	assert( buff != NULL );
	memcpy( block, buff, size );
	(po->offset) += size;
	return true;
}

bool zepto_parse_skip_block( parser_obj* po, uint16_t size )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	if ( po->offset + size > memory_object_get_request_size( po->mem_handle ) )
		return false;
	(po->offset) += size;
	return true;
}

bool zepto_is_parsing_done( parser_obj* po )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	return po->offset >= memory_object_get_request_size( po->mem_handle );
}

uint16_t zepto_parsing_remaining_bytes( parser_obj* po )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	assert( po->offset <= memory_object_get_request_size( po->mem_handle ) );
	return memory_object_get_request_size( po->mem_handle ) - po->offset;
}

////	writing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_write_uint8( REQUEST_REPLY_HANDLE mem_h, uint8_t val )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_append( mem_h, 1 );
	assert( buff != NULL );
	buff[0] = val;
}

void zepto_write_encoded_uint16( REQUEST_REPLY_HANDLE mem_h, uint16_t num )
{
printf( "zepto_write_encoded_uint16( %x )\n", num ); 
if ( num == 0x8000 )
{
	num = 0x8000;
}
	assert( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff;
	if ( num < 128 )
	{
		buff = memory_object_append( mem_h, 1 );
		assert( buff != NULL );
		*buff = (uint8_t)num;
	}
	else if ( num < 16512 )
	{
		buff = memory_object_append( mem_h, 2 );
		assert( buff != NULL );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (num - 128) >> 7;
	}
	else
	{
		buff = memory_object_append( mem_h, 3 );
		assert( buff != NULL );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (( (num - 128) >> 7 ) & 0x7F ) + 128;
		buff[2] = (num - 16512) >> 14;
		printf( "0x%x = %x, %x, %x\n", num, buff[0], buff[1], buff[2] );
	}
}

void zepto_write_block( REQUEST_REPLY_HANDLE mem_h, const uint8_t* block, uint16_t size )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_append( mem_h, size );
	memcpy( buff, block, size );
}

void zepto_response_to_request( MEMORY_HANDLE mem_h )
{
	memory_object_response_to_request( mem_h );
}

void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( po_start->mem_handle == mem_h );
	assert( po_start->mem_handle == po_end->mem_handle );
	assert( po_start->mem_handle < REQUEST_REPLY_OBJ_MAX_CNT ); 
	assert( po_start->offset <= po_end->offset );
	MEMORY_HANDLE ret_handle = po_start->mem_handle;
	po_start->mem_handle = MEMORY_HANDLE_INVALID;
	po_end->mem_handle = MEMORY_HANDLE_INVALID;
	memory_object_cut_and_make_response( ret_handle, po_start->offset, po_end->offset - po_start->offset );
}
/*
void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, uint16_t cutoff_cnt )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( po_start->mem_handle == mem_h );
	assert( po_start->mem_handle < REQUEST_REPLY_OBJ_MAX_CNT ); 
	assert( po_start->offset + cutoff_cnt <= memory_object_get_request_size( mem_h ) );
	MEMORY_HANDLE ret_handle = po_start->mem_handle;
	po_start->mem_handle = MEMORY_HANDLE_INVALID;
	memory_object_cut_and_make_response( ret_handle, po_start->offset, memory_object_get_request_size( mem_h ) - cutoff_cnt - po_start->offset );
}
*/

void zepto_write_prepend_byte( MEMORY_HANDLE mem_h, uint8_t bt )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t b = bt;
	memory_object_prepend( mem_h, &b, 1 );
}

void zepto_write_prepend_block( MEMORY_HANDLE mem_h, const uint8_t* block, uint16_t size )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	memory_object_prepend( mem_h, block, size );
}



// inspired by SAGDP: creating a copy of the packet
uint16_t zepto_writer_get_response_size( MEMORY_HANDLE mem_h )
{
	return memory_object_get_response_size( mem_h );
}

void zepto_writer_get_copy_of_response( MEMORY_HANDLE mem_h, uint8_t* buff )
{
	memcpy( buff, memory_object_get_response_ptr( mem_h ), memory_object_get_response_size( mem_h ) );
}

