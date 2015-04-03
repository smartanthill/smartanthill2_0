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

#define BASE_MEM_BLOCK_SIZE	0x2000
uint8_t BASE_MEM_BLOCK[ BASE_MEM_BLOCK_SIZE ];
request_reply_mem_obj memory_objects[ MEMORY_HANDLE_MAX ]; // fixed size array for a while

#define ASSERT_MEMORY_HANDLE_VALID( h ) assert( h!= MEMORY_HANDLE_INVALID && h < MEMORY_HANDLE_MAX && memory_objects[h].ptr > BASE_MEM_BLOCK && memory_objects[h].ptr < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
#define CHECK_AND_PRINT_INVALID_HANDLE( h ) \
	if ( !(h!= MEMORY_HANDLE_INVALID && h < MEMORY_HANDLE_MAX && memory_objects[h].ptr > BASE_MEM_BLOCK && memory_objects[h].ptr < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE) ) \
	{ \
		printf( "Invalid handle: h = %d", h ); \
		if ( h < MEMORY_HANDLE_MAX ) \
			printf( ", ptr = 0x%x (base = 0x%x)", memory_objects[h].ptr, BASE_MEM_BLOCK );\
		printf( "\n" ); \
	}

// helpers

void zepto_mem_man_write_encoded_uint16_no_size_checks_forward( uint8_t* buff, uint16_t num )
{
	assert( buff != NULL );
	if ( num < 128 )
		*buff = (uint8_t)num;
	else if ( num < 16512 )
	{
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (num - 128) >> 7;
	}
	else
	{
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (( (num - 128) >> 7 ) & 0x7F ) + 128;
		buff[2] = (num - 16512) >> 14;
	}
}

void zepto_mem_man_write_encoded_uint16_no_size_checks_backward( uint8_t* buff, uint16_t num )
{
	assert( buff != NULL );
	if ( num < 128 )
		*buff = (uint8_t)num;
	else if ( num < 16512 )
	{
		*buff = ((uint8_t)num & 0x7F) + 128;
		*(buff-1) = (num - 128) >> 7;
	}
	else
	{
		*buff = ((uint8_t)num & 0x7F) + 128;
		*(buff-1) = (( (num - 128) >> 7 ) & 0x7F ) + 128;
		*(buff-2) = (num - 16512) >> 14;
	}
}

uint16_t zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( uint8_t* buff )
{
	assert( buff != NULL );
	uint16_t ret;
	if ( ( buff[ 0 ] & 128 ) == 0 )
		ret = (uint16_t)(buff[ 0 ]); 
	else if (  ( buff[ 1 ] & 128 ) == 0  )
		ret = 128 + ( (uint16_t)(buff[0] & 0x7F) | ( ((uint16_t)(buff[1])) << 7) ); 
	else
	{
		assert( (buff[2] & 0x80) == 0 );
if ( buff[2] >= 4 )
{
	printf( "Buff[2] = %x\n", buff[2] );
}
		assert( buff[2] < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (((uint16_t)(buff[1])) &0x7F ) << 7) ) | ( ((uint16_t)(buff[2])) << 14);
	}
	return ret;
}

uint16_t zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( uint8_t* buff )
{
	assert( buff != NULL );
	uint16_t ret;
	if ( ( *buff & 128 ) == 0 )
		ret = (uint16_t)(*buff); 
	else if (  ( *(buff-1) & 128 ) == 0  )
		ret = 128 + ( (uint16_t)(*buff & 0x7F) | ( ((uint16_t)(*(buff-1))) << 7) ); 
	else
	{
		assert( (*(buff-2) & 0x80) == 0 );
		assert( *(buff-2) < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(*buff & 0x7F) | ( (((uint16_t)(*(buff-1))) &0x7F ) << 7) ) | ( ((uint16_t)(*(buff-2))) << 14);
	}
	return ret;
}

void zepto_mem_man_check_sanity()
{
	uint8_t i;		
		
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		if ( memory_objects[i].rq_size + memory_objects[i].rsp_size )
			assert( memory_objects[i].ptr != 0 );
	}

	uint8_t first = 0, last = 0;
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
		if ( memory_objects[i].ptr != 0 )
		{
			first = i;
			last = i;
			break;
		}
	
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		if ( memory_objects[i].ptr != 0 )
		{
			if ( memory_objects[ i ].ptr <= memory_objects[ first ].ptr )
				first = i;
			if ( memory_objects[ i ].ptr >= memory_objects[ last ].ptr )
				last = i;
		}
	}

	assert( memory_objects[ first ].ptr > BASE_MEM_BLOCK );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ first ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	assert( free_at_left == memory_objects[ first ].ptr - BASE_MEM_BLOCK );

	assert( memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	assert( memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size + free_at_right == BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );

	uint16_t total_sz = free_at_left;

	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		if ( memory_objects[i].ptr != 0 )
		{
			uint8_t* left_end_of_free_space_at_right = memory_objects[ i ].ptr + memory_objects[ i ].rq_size + memory_objects[ i ].rsp_size;
			uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
			uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;
			uint16_t free_at_right_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right );
			assert( free_at_right == free_at_right_copy );

			total_sz += memory_objects[ i ].rq_size + memory_objects[ i ].rsp_size + free_at_right;

			uint8_t* right_end_of_free_space_at_left = memory_objects[ i ].ptr - 1;
			uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
			uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;
			uint16_t free_at_left_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left );
			assert( free_at_left == free_at_left_copy );
		}
	}

	assert( total_sz == BASE_MEM_BLOCK_SIZE );
}







void zepto_mem_man_init_memory_management()
{
	for ( uint8_t i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		memset( &(memory_objects[i]), 0, sizeof( request_reply_mem_obj ) );
	}
	BASE_MEM_BLOCK[0] = 1;
	memory_objects[ MEMORY_HANDLE_MAIN_LOOP ].ptr = BASE_MEM_BLOCK + 1;
	memory_objects[ MEMORY_HANDLE_MAIN_LOOP ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_MAIN_LOOP ].rsp_size = 0;
	BASE_MEM_BLOCK[1] = 1;
	memory_objects[ MEMORY_HANDLE_SAGDP_LSM ].ptr = BASE_MEM_BLOCK + 2;
	memory_objects[ MEMORY_HANDLE_SAGDP_LSM ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_SAGDP_LSM ].rsp_size = 0;
	BASE_MEM_BLOCK[2] = 1;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].ptr = BASE_MEM_BLOCK + 3;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].rsp_size = 0;
	BASE_MEM_BLOCK[3] = 1;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].ptr = BASE_MEM_BLOCK + 4;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].rsp_size = 0;

	uint16_t remains_at_right = BASE_MEM_BLOCK_SIZE - 4;

	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( BASE_MEM_BLOCK + 4, remains_at_right );
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE - 1, remains_at_right );

	zepto_mem_man_check_sanity();
}

void zepto_mem_man_move_obj_max_right( REQUEST_REPLY_HANDLE mem_h )
{
	if ( memory_objects[ mem_h ].ptr == 0 )
	{
		printf( "handle = %d: ptr == NULL\n", mem_h );
	}
	assert( memory_objects[ mem_h ].ptr != 0 );

	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	assert( free_at_right >= 1 );
	if ( free_at_right == 1 ) return;
	uint16_t move_sz = free_at_right - 1; // we move at maximum; this may or may not be optimal

	uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;

	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;

	memmove( memory_objects[ mem_h ].ptr + move_sz, memory_objects[ mem_h ].ptr, memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size );
	memory_objects[ mem_h ].ptr += move_sz;

	*right_end_of_free_space_at_right = 1; // by construction we have left just 1 free byte (minimal allowed value);
	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left, free_at_left + move_sz );
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( memory_objects[ mem_h ].ptr - 1, free_at_left + move_sz );
}

void zepto_mem_man_move_obj_max_left( REQUEST_REPLY_HANDLE mem_h )
{
	assert( memory_objects[ mem_h ].ptr != 0 );

	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	assert( free_at_left >= 1 );
	if ( free_at_left == 1 ) return;
	uint16_t move_sz = free_at_left - 1; // we move at maximum; this may or may not be optimal

	uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;

	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;

	memmove( memory_objects[ mem_h ].ptr - move_sz, memory_objects[ mem_h ].ptr, memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size );
	memory_objects[ mem_h ].ptr -= move_sz;

	*left_end_of_free_space_at_left = 1; // by construction we have left just 1 free byte (minimal allowed value);
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right, free_at_right + move_sz );
	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size, free_at_right + move_sz );
zepto_mem_man_check_sanity();
}

uint16_t zepto_mem_man_get_freeable_size_at_right( REQUEST_REPLY_HANDLE mem_h )
{
	assert( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	assert( free_at_right >= 1 );
	return free_at_right - 1;
}

uint16_t zepto_mem_man_get_freeable_size_at_left( REQUEST_REPLY_HANDLE mem_h )
{
	assert( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	assert( free_at_left >= 1 );
	return free_at_left - 1;
}

REQUEST_REPLY_HANDLE zepto_mem_man_get_next_block( REQUEST_REPLY_HANDLE mem_h )
{
	assert( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	uint8_t* next_block_start = left_end_of_free_space_at_right + free_at_right;
	assert( next_block_start <= BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
	if ( next_block_start == BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE )
		return MEMORY_HANDLE_INVALID;

	// now just find a proper block and return its handle
	uint8_t i;
	for ( i=mem_h + 1; i<MEMORY_HANDLE_MAX; i++ )
		if ( memory_objects[i].ptr == next_block_start )
			return i;
	for ( i=0; i<mem_h; i++ )
		if ( memory_objects[i].ptr == next_block_start )
			return i;

	zepto_mem_man_check_sanity(); // we can hardly be here
	return MEMORY_HANDLE_INVALID;
}

REQUEST_REPLY_HANDLE zepto_mem_man_get_prev_block( REQUEST_REPLY_HANDLE mem_h )
{
	assert( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	assert( free_at_left >= 1 );
	uint8_t* prev_block_end = right_end_of_free_space_at_left - free_at_left + 1;
	if ( prev_block_end == BASE_MEM_BLOCK )
		return MEMORY_HANDLE_INVALID;

	// now just find a proper block and return its handle
	uint8_t i;
	for ( i=0; i<mem_h; i++ )
		if ( memory_objects[i].ptr + memory_objects[i].rq_size + memory_objects[i].rsp_size == prev_block_end )
			return i;
	for ( i=mem_h + 1; i<MEMORY_HANDLE_MAX; i++ )
		if ( memory_objects[i].ptr + memory_objects[i].rq_size + memory_objects[i].rsp_size == prev_block_end )
			return i;
	zepto_mem_man_check_sanity(); // we can hardly be here
	return MEMORY_HANDLE_INVALID;
}

void zepto_mem_man_move_all_right( REQUEST_REPLY_HANDLE h_right, REQUEST_REPLY_HANDLE h_left )
{
	assert( h_right !=  MEMORY_HANDLE_INVALID );
	assert( h_left !=  MEMORY_HANDLE_INVALID );
	assert( memory_objects[h_left].ptr <= memory_objects[h_right].ptr );
	REQUEST_REPLY_HANDLE h_iter = h_right;
	do
	{
zepto_mem_man_check_sanity();
		zepto_mem_man_move_obj_max_right( h_iter );
zepto_mem_man_check_sanity();
		h_iter = zepto_mem_man_get_prev_block( h_iter );
	}
	while ( h_iter != MEMORY_HANDLE_INVALID && h_iter != h_left );
}

void zepto_mem_man_move_all_left( REQUEST_REPLY_HANDLE h_left, REQUEST_REPLY_HANDLE h_right )
{
	assert( h_right !=  MEMORY_HANDLE_INVALID );
	assert( h_left !=  MEMORY_HANDLE_INVALID );
	assert( memory_objects[h_left].ptr <= memory_objects[h_right].ptr );
	REQUEST_REPLY_HANDLE h_iter = h_left;
	do
	{
		zepto_mem_man_move_obj_max_left( h_iter );
		h_iter = zepto_mem_man_get_next_block( h_iter );
	}
	while ( h_iter != MEMORY_HANDLE_INVALID && h_iter != h_right );
}










uint8_t* memory_object_get_request_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].ptr;
}

uint16_t memory_object_get_request_size( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].rq_size;
}

uint8_t* memory_object_get_response_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size;
}

uint16_t memory_object_get_response_size( REQUEST_REPLY_HANDLE mem_h )
{
	assert( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].rsp_size;
}












uint8_t* zepto_mem_man_try_expand_right( REQUEST_REPLY_HANDLE mem_h, uint16_t size ) 
// if successful, updates memory_objects[ mem_h ].rsp_size -> memory_objects[ mem_h ].rsp_size + size and returns ptr to added part (that is, memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + (former value of 'memory_objects[ mem_h ].rsp_size') )
// if fails, returns NULL
{
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	if ( size >= free_at_right ) return NULL;
	uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;

	left_end_of_free_space_at_right += size;
	free_at_right -= size;
	assert( free_at_right >= 1 );
	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right, free_at_right );
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right, free_at_right );

	uint8_t* ret = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size += size;
	return ret;
}

bool zepto_mem_man_try_expand_left( REQUEST_REPLY_HANDLE mem_h, uint16_t size ) 
// if successful:
//    - updates memory_objects[ mem_h ].ptr -> memory_objects[ mem_h ].ptr - size
//    - updates memory_objects[ mem_h ].rsp_size -> memory_objects[ mem_h ].rsp_size + size
//    - returns true
// if fails
//    - returns false
// NOTE: as prepending is applicable only to response, memory_objects[ mem_h ].rq_size must be set to 0 (with respective other changes) before this call
{
	assert( memory_objects[ mem_h ].rq_size == 0 );

	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	if ( size >= free_at_left ) return false;
	uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;

	right_end_of_free_space_at_left -= size;
	free_at_left -= size;
	assert( free_at_left >= 1 );
	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left, free_at_left );
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left, free_at_left );

	memory_objects[ mem_h ].ptr -= size;
	memory_objects[ mem_h ].rsp_size += size;
	return true;
}

uint8_t* zepto_mem_man_try_move_left_expand_right( REQUEST_REPLY_HANDLE mem_h, uint16_t size ) 
// if successful, updates memory_objects[ mem_h ].rsp_size -> memory_objects[ mem_h ].rsp_size + size and returns ptr to added part (that is, memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + (former value of 'memory_objects[ mem_h ].rsp_size') )
// if fails, returns NULL
{
	uint16_t freeable = zepto_mem_man_get_freeable_size_at_right( mem_h ) + zepto_mem_man_get_freeable_size_at_left( mem_h );
	if ( size > freeable ) return NULL;

	zepto_mem_man_move_obj_max_left( mem_h );
zepto_mem_man_check_sanity();
	assert( size <= zepto_mem_man_get_freeable_size_at_right( mem_h ) );
	uint8_t* ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	assert( ret != NULL );
	return ret;
}

bool zepto_mem_man_try_move_right_expand_left( REQUEST_REPLY_HANDLE mem_h, uint16_t size ) 
// if successful:
//    - updates memory_objects[ mem_h ].ptr -> memory_objects[ mem_h ].ptr + size
//    - updates memory_objects[ mem_h ].rq_size -> memory_objects[ mem_h ].rq_size + size
//    - returns true
// if fails
//    - returns false
// NOTE: as prepending is applicable only to response, memory_objects[ mem_h ].rq_size must be set to 0 (with respective other changes) before this call
//    - returns true
// if fails
//    - returns false
// NOTE: as prepending is applicable only to response, memory_objects[ mem_h ].rq_size must be set to 0 (with respective other changes) before this call
{
	uint16_t freeable = zepto_mem_man_get_freeable_size_at_right( mem_h ) + zepto_mem_man_get_freeable_size_at_left( mem_h );
	if ( size > freeable ) return false;

	zepto_mem_man_move_obj_max_right( mem_h );
	assert( size <= zepto_mem_man_get_freeable_size_at_left( mem_h ) );
	bool ret = zepto_mem_man_try_expand_left( mem_h, size );
	assert( ret );
	return ret;
}

uint16_t zepto_mem_man_get_total_freeable_space_at_right( REQUEST_REPLY_HANDLE mem_h, REQUEST_REPLY_HANDLE* h_very_right, uint16_t desired_size )
{
	REQUEST_REPLY_HANDLE h_iter = mem_h;
	uint16_t free_at_right = 0;
	while ( free_at_right < desired_size && h_iter != MEMORY_HANDLE_INVALID )
	{
		*h_very_right = h_iter;
		free_at_right += zepto_mem_man_get_freeable_size_at_right( h_iter );
		h_iter = zepto_mem_man_get_next_block( h_iter );
	}
	return free_at_right;
}

uint16_t zepto_mem_man_get_total_freeable_space_at_left( REQUEST_REPLY_HANDLE mem_h, REQUEST_REPLY_HANDLE* h_very_left, uint16_t desired_size )
{
	REQUEST_REPLY_HANDLE h_iter = mem_h;
	uint16_t free_at_left = 0;
	while ( free_at_left < desired_size && h_iter != MEMORY_HANDLE_INVALID )
	{
		*h_very_left = h_iter;
		free_at_left += zepto_mem_man_get_freeable_size_at_left( h_iter );
		h_iter = zepto_mem_man_get_prev_block( h_iter );
	}
	return free_at_left;
}



uint8_t* memory_object_append( REQUEST_REPLY_HANDLE mem_h, uint16_t size )
{
zepto_mem_man_check_sanity();
	uint8_t* ret;
	
	ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret != NULL ) return ret;

	ret = zepto_mem_man_try_move_left_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret != NULL ) return ret;

	REQUEST_REPLY_HANDLE last_at_right;
	uint16_t freeable_at_right = zepto_mem_man_get_total_freeable_space_at_right( mem_h, &last_at_right, size );
	assert( last_at_right != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_right( last_at_right, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
		assert( ret );
		return ret;
	}

	REQUEST_REPLY_HANDLE last_at_left;
	uint16_t freeable_at_left = zepto_mem_man_get_total_freeable_space_at_left( mem_h, &last_at_left, size );
	assert( last_at_left != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_left( last_at_left, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_left + freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_move_left_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
		assert( ret ); // TODO: yet to be considered: forced truncation and further error handling
		return ret;
	}
	else
	{
		assert( 0 == "insufficient memory in memory_object_append()" );
		return 0;
	}
}

void memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, const uint8_t* buff, uint16_t size )
{
zepto_mem_man_check_sanity();
	if ( memory_objects[ mem_h ].rq_size )
	{
		uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
		uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
		uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;

		right_end_of_free_space_at_left += memory_objects[ mem_h ].rq_size;
		free_at_left += memory_objects[ mem_h ].rq_size;
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left, free_at_left );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left, free_at_left );

		memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
		memory_objects[ mem_h ].rq_size = 0;
	}
zepto_mem_man_check_sanity();
	bool ret;
	
	ret = zepto_mem_man_try_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret )
	{
		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return;
	}

	ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret )
	{
		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return;
	}

	REQUEST_REPLY_HANDLE last_at_right;
	uint16_t freeable_at_right = zepto_mem_man_get_total_freeable_space_at_right( mem_h, &last_at_right, size );
	assert( last_at_right != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_right( last_at_right, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
		assert( ret );
		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return;
	}

	REQUEST_REPLY_HANDLE last_at_left;
	uint16_t freeable_at_left = zepto_mem_man_get_total_freeable_space_at_left( mem_h, &last_at_left, size );
	assert( last_at_left != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_left( last_at_left, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_left + freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
		assert( ret ); // TODO: yet to be considered: forced truncation and further error handling
		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return;
	}
}


/*
uint8_t* memory_object_append( REQUEST_REPLY_HANDLE mem_h, uint16_t size )
{
printf( "memory_object_append( %d )\n", size ); 
	// memory at right must be acquired
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;
	assert( free_at_right == zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right ) );
	if ( size < free_at_right ) // easy and happy case: no mem moving; note that we need at least 1-byte free space as a result
	{
		left_end_of_free_space_at_right += size;
		free_at_right -= size;
		assert( free_at_right >= 1 );
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right, free_at_right );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right, free_at_right );

		uint8_t* ret = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
		memory_objects[ mem_h ].rsp_size += size;
		return ret;
	}
	else
	{
		uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
		uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
		uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;
		assert( free_at_left == zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left ) );
		if ( size <= free_at_right + free_at_left - 2 ) // second easy case: moving only a current block; note that we need at least 1-byte free space at both sides as a result
		{
			assert( free_at_left <= size );
			uint16_t move_sz = size - free_at_left + 1;
			assert( move_sz < free_at_left );
			assert( free_at_right + move_sz > size );

			memmove( memory_objects[ mem_h ].ptr - move_sz, memory_objects[ mem_h ].ptr, memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size );
			memory_objects[ mem_h ].ptr -= move_sz;

			zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left, free_at_left - move_sz );
			zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right, free_at_right - size + move_sz );

			zepto_mem_man_write_encoded_uint16_no_size_checks_backward( memory_objects[ mem_h ].ptr - 1, free_at_left - move_sz );
			zepto_mem_man_write_encoded_uint16_no_size_checks_forward( memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size, free_at_right - size + move_sz );

			uint8_t* ret = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
			memory_objects[ mem_h ].rsp_size += size;
			return ret;
		}
		else // now we cannot avoid complex memory moves
		{
			REQUEST_REPLY_HANDLE h_iter = mem_h;
			REQUEST_REPLY_HANDLE h_last = mem_h;
			uint16_t free_at_right = 0;
			while ( free_at_right < size && h_iter != MEMORY_HANDLE_INVALID )
			{
				h_last = h_iter;
				free_at_right += zepto_mem_man_get_freeable_size_at_right( h_iter );
				h_iter = zepto_mem_man_get_next_block( h_iter );
			}

			zepto_mem_man_move_all_right( h_last, mem_h );

			if ( free_at_right < size )
			{
				uint16_t size_rem = size - free_at_right;
				REQUEST_REPLY_HANDLE h_iter = mem_h;
				REQUEST_REPLY_HANDLE h_last = mem_h;
				uint16_t free_at_left = 0;
				while ( free_at_left < size_rem && h_iter != MEMORY_HANDLE_INVALID )
				{
					h_last = h_iter;
					free_at_left += zepto_mem_man_get_freeable_size_at_left( h_iter );
					h_iter = zepto_mem_man_get_prev_block( h_iter );
				}

				free_at_right += free_at_left;

				zepto_mem_man_move_all_left( h_last, mem_h );
			}

			assert( free_at_right >= size ); // TODO: add forced freeing
		}
	}
}

void memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, const uint8_t* buff, uint16_t size )
{
	// TODO: we have a few options here w/resp to request: to kill it, or to ensure that there is no request, or to move responce accordingly

	memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
	memory_objects[ mem_h ].rq_size = 0;
	memory_objects[ mem_h ].ptr -= size;
	// TODO: the above line may require further actions for "getting" memory
	memory_objects[ mem_h ].rsp_size += size;
	memcpy( memory_objects[ mem_h ].ptr, buff, size );
}
*/
void memory_object_cut_and_make_response( REQUEST_REPLY_HANDLE mem_h, uint16_t offset, uint16_t size )
{
zepto_mem_man_check_sanity();
	// memory at left and at right must be released
	assert( offset <= memory_objects[ mem_h ].rq_size );
	assert( offset + size <= memory_objects[ mem_h ].rq_size );

	uint16_t freeing_at_left = offset;
	if ( freeing_at_left )
	{
		uint8_t* right_end_of_free_space = memory_objects[ mem_h ].ptr - 1;
		uint16_t sz_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space );
		uint8_t* left_end_of_free_space = right_end_of_free_space - sz_at_left + 1;
		uint16_t sz_at_left_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space );
		assert( sz_at_left == sz_at_left_copy );
		sz_at_left += freeing_at_left;
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space, sz_at_left );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space + freeing_at_left, sz_at_left );
	}

	uint16_t freeing_at_right = memory_objects[ mem_h ].rsp_size + ( memory_objects[ mem_h ].rq_size - offset - size );
	if ( freeing_at_right )
	{
		uint8_t* left_end_of_free_space = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
		uint16_t sz_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space );
		uint8_t* right_end_of_free_space = left_end_of_free_space + sz_at_right - 1;
		uint16_t sz_at_right_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space );
		assert( sz_at_right == sz_at_right_copy );
		sz_at_right += freeing_at_right;
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space - freeing_at_right, sz_at_right );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space, sz_at_right );
	}

	memory_objects[ mem_h ].ptr += offset;
	// TODO: the above line may require further actions for "returning" memory
	memory_objects[ mem_h ].rq_size = 0;
	memory_objects[ mem_h ].rsp_size = size;
	// TODO: the above line may require further actions for "returning" memory
zepto_mem_man_check_sanity();
}

void memory_object_response_to_request( REQUEST_REPLY_HANDLE mem_h )
{
zepto_mem_man_check_sanity();
	// former request is no longer necessary, and memory must be released
	uint16_t freeing_at_left = memory_objects[ mem_h ].rq_size;
	if ( freeing_at_left )
	{
		uint8_t* right_end_of_free_space = memory_objects[ mem_h ].ptr - 1;
		uint16_t sz_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space );
		uint8_t* left_end_of_free_space = right_end_of_free_space - sz_at_left + 1;
		uint16_t sz_at_left_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space );
		assert( sz_at_left == sz_at_left_copy );
		sz_at_left += freeing_at_left;
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space, sz_at_left );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space + freeing_at_left, sz_at_left );
	}

	memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
	memory_objects[ mem_h ].rq_size = memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size = 0;
zepto_mem_man_check_sanity();

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
printf( "zepto_parse_encoded_uint16(): ini offset = %d...", po->offset ); 
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
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (((uint16_t)(buff[1])) &0x7F ) << 7) ) | ( ((uint16_t)(buff[2])) << 14);
		po->offset += 3;
	}
	printf( "new offset = %d, num = %x (%x, %x, %x)\n", po->offset, ret, buff[0], buff[1], buff[2] ); 
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
	assert( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	assert( po_start->offset <= po_end->offset );
	MEMORY_HANDLE ret_handle = po_start->mem_handle;
	po_start->mem_handle = MEMORY_HANDLE_INVALID;
	po_end->mem_handle = MEMORY_HANDLE_INVALID;
	memory_object_cut_and_make_response( ret_handle, po_start->offset, po_end->offset - po_start->offset );
}

void zepto_copy_response_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h )
{
	assert( mem_h != target_mem_h );
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( target_mem_h != MEMORY_HANDLE_INVALID );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( mem_h ) + memory_object_get_request_size( mem_h );
	assert( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, memory_object_get_response_size( mem_h ) );
	memcpy( dest_buff, src_buff, memory_object_get_response_size( mem_h ) );
}

void zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h )
{
	assert( mem_h != target_mem_h );
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( target_mem_h != MEMORY_HANDLE_INVALID );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( mem_h );
	assert( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, memory_object_get_request_size( mem_h ) );
	memcpy( dest_buff, src_buff, memory_object_get_request_size( mem_h ) );
}


void zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end, MEMORY_HANDLE target_mem_h )
{
	assert( mem_h != target_mem_h );
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( po_start->mem_handle == mem_h );
	assert( po_start->mem_handle == po_end->mem_handle );
	assert( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	assert( po_start->offset <= po_end->offset );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( po_start->mem_handle ) + po_start->offset;
	assert( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, po_end->offset - po_start->offset );
	memcpy( dest_buff, src_buff, po_end->offset - po_start->offset );
}

/*
void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, uint16_t cutoff_cnt )
{
	assert( mem_h != MEMORY_HANDLE_INVALID );
	assert( po_start->mem_handle == mem_h );
	assert( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
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
/*
uint16_t zepto_writer_get_response_size( MEMORY_HANDLE mem_h )
{
	return memory_object_get_response_size( mem_h );
}

void zepto_writer_get_copy_of_response( MEMORY_HANDLE mem_h, uint8_t* buff )
{
	memcpy( buff, memory_object_get_response_ptr( mem_h ), memory_object_get_response_size( mem_h ) );
}
*/

// specific encoded uint functions
void shift_right_7( uint8_t* num_bytes, uint8_t cnt )
{
	uint8_t i=0;
	for ( ; i<cnt-1;i++)
	{
		num_bytes[i] >>= 7;
		num_bytes[i] |= (num_bytes[i+1] & 0x7f) << 1;
	}
	num_bytes[i] >>= 7;
}

void subtract_1( uint8_t* num_bytes, uint8_t cnt )
{
	uint8_t i=0;
	for ( ; i<cnt;i++)
	{
		num_bytes[i] -= 1;
		if ( num_bytes[i] != 0xFF )
			return;
	}
	assert( 0 == "invalid subtraction" );
}

bool is_less_128( const uint8_t* num_bytes, uint8_t cnt )
{
	uint8_t i=cnt-1;
	for ( ; i; i--)
	{
		if ( num_bytes[i] ) return false;
	}
	return num_bytes[0] < 128;
}


void zepto_parser_encode_uint( const uint8_t* num_bytes, uint8_t num_sz_max, uint8_t** bytes_out )
{
	assert( num_sz_max ); 
	while ( num_sz_max && ( num_bytes[ num_sz_max - 1 ] == 0 ) ) num_sz_max--;
	if ( num_sz_max == 0 )
	{
		**bytes_out = 0;
		(*bytes_out)++;
		return;
	}

	uint8_t buff[16];
	memcpy( buff, num_bytes, num_sz_max );

	for(;;)
	{
		**bytes_out = *buff & 0x7F;
		if ( is_less_128( buff, num_sz_max ) ) 
		{
			(*bytes_out)++;
			return;
		}
		**bytes_out |= 128;
		( *bytes_out )++;
		shift_right_7( buff, num_sz_max );
		subtract_1( buff, num_sz_max );
	}
/*	const uint8_t* num_bytes_end = num_bytes + num_sz_max;
	uint16_t interm = *num_bytes;
//	unsigned int interm = *num_bytes;
	num_bytes++;
	**bytes_out = interm & 0x7F;
	if ( interm < 128 && num_bytes == num_bytes_end ) 
	{
		(*bytes_out)++;
		return;
	}
	**bytes_out |= 128;
	(*bytes_out)++;
	interm >>= 7;

	for ( uint8_t i=1; i<8; i++ )
	{
		if ( num_bytes < num_bytes_end ) 
		{
			interm += ((uint16_t)(*num_bytes)) << i;
//			interm += ((uint16_t)(*num_bytes)) << 1;
//			interm += ((uint16_t)(*num_bytes));
			num_bytes++;
		}
		interm -= 1;
		**bytes_out = interm & 0x7F;
		if ( interm < 128 && num_bytes == num_bytes_end )
		{
			(*bytes_out)++;
			return;
		}
		**bytes_out |= 128;
		(*bytes_out)++;
		interm >>= 7;
//		interm -= 1;
	}
	*/
}

void zepto_parser_decode_uint( uint8_t** packed_num_bytes, uint8_t* bytes_out, uint8_t target_size )
{
	assert( target_size != 0 );
	assert( target_size <= 8 ); // TODO: implement and test for larger sizes
	memset( bytes_out, 0, target_size );
	uint8_t* bytes_out_start = bytes_out;
	uint16_t interm = (**packed_num_bytes) & 0x7F;
	if ( (**packed_num_bytes & 0x80) == 0 )
	{
		*bytes_out = (uint8_t)interm;
		bytes_out++;
		(*packed_num_bytes)++;
		return;
	}
	(*packed_num_bytes)++;
	interm += 128;
	interm += ((uint16_t)( **packed_num_bytes & 0x7F )) << 7;
	*bytes_out = (uint8_t)interm;
	bytes_out++;
	interm >>= 8;

	for ( uint8_t i=1; i<8; i++ )
	{
		if ( (**packed_num_bytes & 0x80) == 0 )
		{
			*bytes_out = (uint8_t)interm;
			bytes_out++;
			assert( bytes_out - bytes_out_start < 8); // TODO: implement and test for larger sizes
			(*packed_num_bytes)++;
			return;
		}
		assert( bytes_out - bytes_out_start < 8); // TODO: implement and test for larger sizes
		(*packed_num_bytes)++;
//		interm += 128;
		interm += (1+(uint16_t)( **packed_num_bytes & 0x7F )) << (7-i);
		*bytes_out = (uint8_t)interm;
		bytes_out++;
		assert( bytes_out - bytes_out_start < 8); // TODO: implement and test for larger sizes
		interm >>= 8;
	}
}

void zepto_parser_decode_uint( parser_obj* po, uint8_t* bytes_out, uint8_t target_size )
{
	assert( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	assert( buff != NULL );
	uint8_t* end = buff;
	zepto_parser_decode_uint( &end, bytes_out, target_size );
	po->offset += end - buff;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t ugly_hook_get_request_size( REQUEST_REPLY_HANDLE mem_h )
{
	return memory_object_get_request_size( mem_h );
}

uint16_t ugly_hook_get_response_size( REQUEST_REPLY_HANDLE mem_h )
{
	return memory_object_get_response_size( mem_h );
}
