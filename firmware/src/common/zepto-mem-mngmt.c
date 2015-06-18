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

typedef struct _request_reply_mem_obj
{ 
	uint8_t* ptr; // TODO: switch to offfset
	uint16_t rq_size;
	uint16_t rsp_size;
} request_reply_mem_obj;

#define BASE_MEM_BLOCK_SIZE	0xA0
uint8_t BASE_MEM_BLOCK[ BASE_MEM_BLOCK_SIZE ];
request_reply_mem_obj memory_objects[ MEMORY_HANDLE_MAX ]; // fixed size array for a while

#define ASSERT_MEMORY_HANDLE_VALID( h ) ZEPTO_DEBUG_ASSERT( h!= MEMORY_HANDLE_INVALID && h < MEMORY_HANDLE_MAX && memory_objects[h].ptr > BASE_MEM_BLOCK && memory_objects[h].ptr < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
#define CHECK_AND_PRINT_INVALID_HANDLE( h ) \
	if ( !(h!= MEMORY_HANDLE_INVALID && h < MEMORY_HANDLE_MAX && memory_objects[h].ptr > BASE_MEM_BLOCK && memory_objects[h].ptr < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE) ) \
	{ \
		ZEPTO_DEBUG_PRINTF_2( "Invalid handle: h = %d", h ); \
		if ( h < MEMORY_HANDLE_MAX ) \
			ZEPTO_DEBUG_PRINTF_3( ", ptr = 0x%x (base = 0x%x)", memory_objects[h].ptr, BASE_MEM_BLOCK );\
		ZEPTO_DEBUG_PRINTF_1( "\n" ); \
	}

// helpers

void zepto_mem_man_write_encoded_uint16_no_size_checks_forward( uint8_t* buff, uint16_t num )
{
	ZEPTO_DEBUG_ASSERT( buff != NULL );
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
	ZEPTO_DEBUG_ASSERT( buff != NULL );
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
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	uint16_t ret;
	if ( ( buff[ 0 ] & 128 ) == 0 )
		ret = (uint16_t)(buff[ 0 ]); 
	else if (  ( buff[ 1 ] & 128 ) == 0  )
		ret = 128 + ( (uint16_t)(buff[0] & 0x7F) | ( ((uint16_t)(buff[1])) << 7) ); 
	else
	{
		ZEPTO_DEBUG_ASSERT( (buff[2] & 0x80) == 0 );
if ( buff[2] >= 4 )
{
	ZEPTO_DEBUG_PRINTF_2( "Buff[2] = %x\n", buff[2] );
}
		ZEPTO_DEBUG_ASSERT( buff[2] < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (((uint16_t)(buff[1])) &0x7F ) << 7) ) | ( ((uint16_t)(buff[2])) << 14);
	}
	return ret;
}

uint16_t zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( uint8_t* buff )
{
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	uint16_t ret;
	if ( ( *buff & 128 ) == 0 )
		ret = (uint16_t)(*buff); 
	else if (  ( *(buff-1) & 128 ) == 0  )
		ret = 128 + ( (uint16_t)(*buff & 0x7F) | ( ((uint16_t)(*(buff-1))) << 7) ); 
	else
	{
		ZEPTO_DEBUG_ASSERT( (*(buff-2) & 0x80) == 0 );
		ZEPTO_DEBUG_ASSERT( *(buff-2) < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(*buff & 0x7F) | ( (((uint16_t)(*(buff-1))) &0x7F ) << 7) ) | ( ((uint16_t)(*(buff-2))) << 14);
	}
	return ret;
}


#ifdef _DEBUG
uint16_t zepto_mem_man_ever_reached = 0;

void zepto_mem_man_print_mem_stats()
{
	uint8_t i;
	uint16_t total_mem = 0;
	ZEPTO_DEBUG_PRINTF_1( "Memory stats:\n" );
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		ZEPTO_DEBUG_PRINTF_6( "[%d] @[+%d (0x%02x)]:\t%d\t%d\n", i, i, (uint16_t)(memory_objects[i].ptr - BASE_MEM_BLOCK), memory_objects[i].rq_size, memory_objects[i].rsp_size );
		total_mem += memory_objects[i].rq_size + memory_objects[i].rsp_size;
	}
	ZEPTO_DEBUG_PRINTF_3( "Size actually used: %d bytes (%d bytes max)\n\n", total_mem, zepto_mem_man_ever_reached );
}

void zepto_mem_man_update_ever_reached()
{
	uint8_t i;
	uint16_t total_mem = 0;
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		total_mem += memory_objects[i].rq_size + memory_objects[i].rsp_size;
	}
	if ( zepto_mem_man_ever_reached < total_mem ) zepto_mem_man_ever_reached = total_mem;
}

void zepto_mem_man_check_sanity()
{
	uint8_t i;		
		
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		if ( memory_objects[i].rq_size + memory_objects[i].rsp_size )
			ZEPTO_DEBUG_ASSERT( memory_objects[i].ptr != 0 );
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

	ZEPTO_DEBUG_ASSERT( memory_objects[ first ].ptr > BASE_MEM_BLOCK );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ first ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	ZEPTO_DEBUG_ASSERT( free_at_left == memory_objects[ first ].ptr - BASE_MEM_BLOCK );

	ZEPTO_DEBUG_ASSERT( memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size < BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	ZEPTO_DEBUG_ASSERT( memory_objects[ last ].ptr + memory_objects[ last ].rq_size + memory_objects[ last ].rsp_size + free_at_right == BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );

	uint16_t total_sz = free_at_left;

	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
	{
		if ( memory_objects[i].ptr != 0 )
		{
			uint8_t* left_end_of_free_space_at_right = memory_objects[ i ].ptr + memory_objects[ i ].rq_size + memory_objects[ i ].rsp_size;
			uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
			uint8_t* right_end_of_free_space_at_right = left_end_of_free_space_at_right + free_at_right - 1;
			uint16_t free_at_right_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_right );
			ZEPTO_DEBUG_ASSERT( free_at_right == free_at_right_copy );

			total_sz += memory_objects[ i ].rq_size + memory_objects[ i ].rsp_size + free_at_right;

			uint8_t* right_end_of_free_space_at_left = memory_objects[ i ].ptr - 1;
			uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
			uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;
			uint16_t free_at_left_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_left );
			ZEPTO_DEBUG_ASSERT( free_at_left == free_at_left_copy );
		}
	}

	ZEPTO_DEBUG_ASSERT( total_sz == BASE_MEM_BLOCK_SIZE );

	zepto_mem_man_update_ever_reached();
}
#else // _DEBUG
void zepto_mem_man_print_mem_stats(){}
void zepto_mem_man_update_ever_reached(){}
void zepto_mem_man_check_sanity(){}
#endif // _DEBUG







void zepto_mem_man_init_memory_management()
{
	uint8_t i;
	for ( i=0; i<MEMORY_HANDLE_MAX; i++ )
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
	memory_objects[ MEMORY_HANDLE_ADDITIONAL_ANSWER ].ptr = BASE_MEM_BLOCK + 3;
	memory_objects[ MEMORY_HANDLE_ADDITIONAL_ANSWER ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_ADDITIONAL_ANSWER ].rsp_size = 0;
	BASE_MEM_BLOCK[3] = 1;
	memory_objects[ MEMORY_HANDLE_DEFAULT_PLUGIN ].ptr = BASE_MEM_BLOCK + 4;
	memory_objects[ MEMORY_HANDLE_DEFAULT_PLUGIN ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_DEFAULT_PLUGIN ].rsp_size = 0;
	BASE_MEM_BLOCK[4] = 1;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].ptr = BASE_MEM_BLOCK + 5;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_TEST_SUPPORT ].rsp_size = 0;
	BASE_MEM_BLOCK[5] = 1;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].ptr = BASE_MEM_BLOCK + 6;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].rq_size = 0;
	memory_objects[ MEMORY_HANDLE_DBG_TMP ].rsp_size = 0;

	uint16_t remains_at_right = BASE_MEM_BLOCK_SIZE - 6;

	zepto_mem_man_write_encoded_uint16_no_size_checks_forward( BASE_MEM_BLOCK + 6, remains_at_right );
	zepto_mem_man_write_encoded_uint16_no_size_checks_backward( BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE - 1, remains_at_right );

	zepto_mem_man_check_sanity();
}

void zepto_mem_man_move_obj_max_right( REQUEST_REPLY_HANDLE mem_h )
{
	if ( memory_objects[ mem_h ].ptr == 0 )
	{
		ZEPTO_DEBUG_PRINTF_2( "handle = %d: ptr == NULL\n", mem_h );
	}
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );

	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	ZEPTO_DEBUG_ASSERT( free_at_right >= 1 );
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
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );

	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	ZEPTO_DEBUG_ASSERT( free_at_left >= 1 );
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
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	ZEPTO_DEBUG_ASSERT( free_at_right >= 1 );
	return free_at_right - 1;
}

uint16_t zepto_mem_man_get_freeable_size_at_left( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	ZEPTO_DEBUG_ASSERT( free_at_left >= 1 );
	return free_at_left - 1;
}

REQUEST_REPLY_HANDLE zepto_mem_man_get_next_block( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* left_end_of_free_space_at_right = memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size + memory_objects[ mem_h ].rsp_size;
	uint16_t free_at_right = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space_at_right );
	uint8_t* next_block_start = left_end_of_free_space_at_right + free_at_right;
	ZEPTO_DEBUG_ASSERT( next_block_start <= BASE_MEM_BLOCK + BASE_MEM_BLOCK_SIZE );
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
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].ptr != 0 );
	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	ZEPTO_DEBUG_ASSERT( free_at_left >= 1 );
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
	ZEPTO_DEBUG_ASSERT( h_right !=  MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( h_left !=  MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( memory_objects[h_left].ptr <= memory_objects[h_right].ptr );
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
	ZEPTO_DEBUG_ASSERT( h_right !=  MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( h_left !=  MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( memory_objects[h_left].ptr <= memory_objects[h_right].ptr );
	REQUEST_REPLY_HANDLE h_iter = h_left;
//	do
	while ( h_iter != MEMORY_HANDLE_INVALID && h_iter != h_right )
	{
		zepto_mem_man_move_obj_max_left( h_iter );
		h_iter = zepto_mem_man_get_next_block( h_iter );
	}
}










uint8_t* memory_object_get_request_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].ptr;
}

uint16_t memory_object_get_request_size( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].rq_size;
}

uint8_t* memory_object_get_response_ptr( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h < MEMORY_HANDLE_MAX );
	return memory_objects[ mem_h ].ptr + memory_objects[ mem_h ].rq_size;
}

uint16_t memory_object_get_response_size( REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h < MEMORY_HANDLE_MAX );
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
	ZEPTO_DEBUG_ASSERT( free_at_right >= 1 );
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
	ZEPTO_DEBUG_ASSERT( memory_objects[ mem_h ].rq_size == 0 );

	uint8_t* right_end_of_free_space_at_left = memory_objects[ mem_h ].ptr - 1;
	uint16_t free_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space_at_left );
	if ( size >= free_at_left ) return false;
	uint8_t* left_end_of_free_space_at_left = right_end_of_free_space_at_left - free_at_left + 1;

	right_end_of_free_space_at_left -= size;
	free_at_left -= size;
	ZEPTO_DEBUG_ASSERT( free_at_left >= 1 );
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
	ZEPTO_DEBUG_ASSERT( size <= zepto_mem_man_get_freeable_size_at_right( mem_h ) );
	uint8_t* ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	ZEPTO_DEBUG_ASSERT( ret != NULL );
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
	ZEPTO_DEBUG_ASSERT( size <= zepto_mem_man_get_freeable_size_at_left( mem_h ) );
	bool ret = zepto_mem_man_try_expand_left( mem_h, size );
	ZEPTO_DEBUG_ASSERT( ret );
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
	if (size == 128)
		size = size;
	
	ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret != NULL ) return ret;

	ret = zepto_mem_man_try_move_left_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret != NULL ) return ret;

	REQUEST_REPLY_HANDLE last_at_right;
	uint16_t freeable_at_right = zepto_mem_man_get_total_freeable_space_at_right( mem_h, &last_at_right, size );
	ZEPTO_DEBUG_ASSERT( last_at_right != MEMORY_HANDLE_INVALID );
zepto_mem_man_print_mem_stats();
	zepto_mem_man_move_all_right( last_at_right, mem_h );
zepto_mem_man_print_mem_stats();
zepto_mem_man_check_sanity();
	if ( freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_expand_right( mem_h, size );
zepto_mem_man_check_sanity();
		ZEPTO_DEBUG_ASSERT( ret );
		return ret;
	}

	REQUEST_REPLY_HANDLE last_at_left;
zepto_mem_man_print_mem_stats();
	uint16_t freeable_at_left = zepto_mem_man_get_total_freeable_space_at_left( mem_h, &last_at_left, size );
	ZEPTO_DEBUG_ASSERT( last_at_left != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_left( last_at_left, mem_h );
zepto_mem_man_print_mem_stats();
zepto_mem_man_check_sanity();
	if ( freeable_at_left + freeable_at_right >= size )
	{
zepto_mem_man_print_mem_stats();
		ret = zepto_mem_man_try_move_left_expand_right( mem_h, size );
#ifdef _DEBUG
zepto_mem_man_check_sanity();
		if ( ret == 0 )
		{
			ZEPTO_DEBUG_PRINTF_3( "memory_object_append(): (re)allocation failed for handle %d and size=%d\n", mem_h, size );
			zepto_mem_man_print_mem_stats();
		}
		ZEPTO_DEBUG_ASSERT( ret ); // TODO: yet to be considered: forced truncation and further error handling
#endif
		return ret;
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( 0 == "insufficient memory in memory_object_append()" );
		return 0;
	}
}

//void memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, const uint8_t* buff, uint16_t size )
uint8_t* memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, uint16_t size )
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
//		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return memory_objects[ mem_h ].ptr;
//		return;
	}

	ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
	if ( ret )
	{
//		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return memory_objects[ mem_h ].ptr;
//		return;
	}

	REQUEST_REPLY_HANDLE last_at_right;
	uint16_t freeable_at_right = zepto_mem_man_get_total_freeable_space_at_right( mem_h, &last_at_right, size );
	ZEPTO_DEBUG_ASSERT( last_at_right != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_right( last_at_right, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
		ZEPTO_DEBUG_ASSERT( ret );
//		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return memory_objects[ mem_h ].ptr;
//		return;
	}

	REQUEST_REPLY_HANDLE last_at_left;
	uint16_t freeable_at_left = zepto_mem_man_get_total_freeable_space_at_left( mem_h, &last_at_left, size );
	ZEPTO_DEBUG_ASSERT( last_at_left != MEMORY_HANDLE_INVALID );
	zepto_mem_man_move_all_left( last_at_left, mem_h );
zepto_mem_man_check_sanity();
	if ( freeable_at_left + freeable_at_right >= size )
	{
		ret = zepto_mem_man_try_move_right_expand_left( mem_h, size );
zepto_mem_man_check_sanity();
		ZEPTO_DEBUG_ASSERT( ret ); // TODO: yet to be considered: forced truncation and further error handling
//		memcpy( memory_objects[ mem_h ].ptr, buff, size );
zepto_mem_man_check_sanity();
		return memory_objects[ mem_h ].ptr;
//		return;
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( 0 );
		return NULL;
	}
}


void memory_object_cut_and_make_response( REQUEST_REPLY_HANDLE mem_h, uint16_t offset, uint16_t size )
{
zepto_mem_man_check_sanity();
	// memory at left and at right must be released
	ZEPTO_DEBUG_ASSERT( offset <= memory_objects[ mem_h ].rq_size );
	ZEPTO_DEBUG_ASSERT( offset + size <= memory_objects[ mem_h ].rq_size );

	uint16_t freeing_at_left = offset;
	if ( freeing_at_left )
	{
		uint8_t* right_end_of_free_space = memory_objects[ mem_h ].ptr - 1;
		uint16_t sz_at_left = zepto_mem_man_parse_encoded_uint16_no_size_checks_backward( right_end_of_free_space );
		uint8_t* left_end_of_free_space = right_end_of_free_space - sz_at_left + 1;
		uint16_t sz_at_left_copy = zepto_mem_man_parse_encoded_uint16_no_size_checks_forward( left_end_of_free_space );
		ZEPTO_DEBUG_ASSERT( sz_at_left == sz_at_left_copy );
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
		ZEPTO_DEBUG_ASSERT( sz_at_right == sz_at_right_copy );
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

void memory_object_request_to_response( REQUEST_REPLY_HANDLE mem_h )
{
	memory_object_cut_and_make_response( mem_h, 0, memory_objects[ mem_h ].rq_size );
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
		ZEPTO_DEBUG_ASSERT( sz_at_left == sz_at_left_copy );
		sz_at_left += freeing_at_left;
		zepto_mem_man_write_encoded_uint16_no_size_checks_forward( left_end_of_free_space, sz_at_left );
		zepto_mem_man_write_encoded_uint16_no_size_checks_backward( right_end_of_free_space + freeing_at_left, sz_at_left );
	}

	memory_objects[ mem_h ].ptr += memory_objects[ mem_h ].rq_size;
	memory_objects[ mem_h ].rq_size = memory_objects[ mem_h ].rsp_size;
	memory_objects[ mem_h ].rsp_size = 0;
zepto_mem_man_check_sanity();

}

void memory_object_free( REQUEST_REPLY_HANDLE mem_h )
{
	// TODO: make sure such implementation is optimal
	memory_object_request_to_response( mem_h );
	memory_object_request_to_response( mem_h );
}













////	parsing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_parser_init( parser_obj* po, REQUEST_REPLY_HANDLE mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	po->mem_handle = mem_h;
	po->offset = 0;
}

void zepto_parser_init_by_parser( parser_obj* po, const parser_obj* po_base )
{
	ZEPTO_DEBUG_ASSERT( po_base->mem_handle != MEMORY_HANDLE_INVALID );
	po->mem_handle = po_base->mem_handle;
	po->offset = po_base->offset;
}

uint8_t zepto_parse_uint8( parser_obj* po )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	uint8_t ret = buff[ 0 ];
	( po->offset ) ++;
	return ret;
}
/*
uint16_t zepto_parse_encoded_uint16( parser_obj* po )
{
ZEPTO_DEBUG_PRINTF_2( "zepto_parse_encoded_uint16(): ini offset = %d...", po->offset ); 
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	ZEPTO_DEBUG_ASSERT( buff != NULL );
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
//	else if (buff[0] == 0x80 && buff[1] == 0xff && buff[2] == 1 )
//	{
//		ret = 0x8000;
//		po->offset += 3;
//	}
	else
	{
		ZEPTO_DEBUG_ASSERT( (buff[2] & 0x80) == 0 );
		ZEPTO_DEBUG_ASSERT( buff[2] < 4 ); // to stay within uint16_max
		ret = 16512 + ( (uint16_t)(buff[0] & 0x7F) | ( (((uint16_t)(buff[1])) &0x7F ) << 7) ) | ( ((uint16_t)(buff[2])) << 14);
		po->offset += 3;
	}
	ZEPTO_DEBUG_PRINTF_5( "new offset = %d, num = %x (%x, %x, %x)\n", po->offset, ret, buff[0], buff[1], buff[2] ); 
	return ret;
}
*/
bool zepto_parse_read_block( parser_obj* po, uint8_t* block, uint16_t size )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	if ( po->offset + size > memory_object_get_request_size( po->mem_handle ) )
		return false;
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	memcpy( block, buff, size );
	(po->offset) += size;
	return true;
}

bool zepto_parse_skip_block( parser_obj* po, uint16_t size )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	if ( po->offset + size > memory_object_get_request_size( po->mem_handle ) )
		return false;
	(po->offset) += size;
	return true;
}

bool zepto_is_parsing_done( parser_obj* po )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	return po->offset >= memory_object_get_request_size( po->mem_handle );
}

uint16_t zepto_parsing_remaining_bytes( parser_obj* po )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
if ( !(po->offset <= memory_object_get_request_size( po->mem_handle ) ) )
{
	ZEPTO_DEBUG_PRINTF_4( "zepto_parsing_remaining_bytes(): mem_h = %d, offset = %d, rq_sz = %d\n", po->mem_handle, po->offset, memory_object_get_request_size( po->mem_handle ) );
}
	ZEPTO_DEBUG_ASSERT( po->offset <= memory_object_get_request_size( po->mem_handle ) );
	return memory_object_get_request_size( po->mem_handle ) - po->offset;
}

////	writing functions	//////////////////////////////////////////////////////////////////////////////////////

void zepto_write_uint8( REQUEST_REPLY_HANDLE mem_h, uint8_t val )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_append( mem_h, 1 );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	buff[0] = val;
}
/*
void zepto_write_encoded_uint16( REQUEST_REPLY_HANDLE mem_h, uint16_t num )
{
ZEPTO_DEBUG_PRINTF_2( "zepto_write_encoded_uint16( %x )\n", num ); 
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff;
	if ( num < 128 )
	{
		buff = memory_object_append( mem_h, 1 );
		ZEPTO_DEBUG_ASSERT( buff != NULL );
		*buff = (uint8_t)num;
	}
	else if ( num < 16512 )
	{
		buff = memory_object_append( mem_h, 2 );
		ZEPTO_DEBUG_ASSERT( buff != NULL );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (num - 128) >> 7;
	}
	else
	{
		buff = memory_object_append( mem_h, 3 );
		ZEPTO_DEBUG_ASSERT( buff != NULL );
		buff[0] = ((uint8_t)num & 0x7F) + 128;
		buff[1] = (( (num - 128) >> 7 ) & 0x7F ) + 128;
		buff[2] = (num - 16512) >> 14;
		ZEPTO_DEBUG_PRINTF_5( "0x%x = %x, %x, %x\n", num, buff[0], buff[1], buff[2] );
	}
}
*/
void zepto_write_block( REQUEST_REPLY_HANDLE mem_h, const uint8_t* block, uint16_t size )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_append( mem_h, size );
	memcpy( buff, block, size );
}

void zepto_response_to_request( MEMORY_HANDLE mem_h )
{
	memory_object_response_to_request( mem_h );
}

void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == mem_h );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == po_end->mem_handle );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	ZEPTO_DEBUG_ASSERT( po_start->offset <= po_end->offset );
	MEMORY_HANDLE ret_handle = po_start->mem_handle;
	po_start->mem_handle = MEMORY_HANDLE_INVALID;
	po_end->mem_handle = MEMORY_HANDLE_INVALID;
	memory_object_cut_and_make_response( ret_handle, po_start->offset, po_end->offset - po_start->offset );
}

void zepto_copy_response_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != target_mem_h );
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( target_mem_h != MEMORY_HANDLE_INVALID );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( mem_h ) + memory_object_get_request_size( mem_h );
	ZEPTO_DEBUG_ASSERT( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, memory_object_get_response_size( mem_h ) );
	memcpy( dest_buff, src_buff, memory_object_get_response_size( mem_h ) );
}

void zepto_append_response_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != target_mem_h );
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( target_mem_h != MEMORY_HANDLE_INVALID );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( mem_h ) + memory_object_get_request_size( mem_h );
	ZEPTO_DEBUG_ASSERT( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, memory_object_get_response_size( mem_h ) );
	memcpy( dest_buff, src_buff, memory_object_get_response_size( mem_h ) );
}

void zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != target_mem_h );
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( target_mem_h != MEMORY_HANDLE_INVALID );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( mem_h );
	ZEPTO_DEBUG_ASSERT( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, memory_object_get_request_size( mem_h ) );
	memcpy( dest_buff, src_buff, memory_object_get_request_size( mem_h ) );
}


void zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end, MEMORY_HANDLE target_mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != target_mem_h );
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == mem_h );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == po_end->mem_handle );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	ZEPTO_DEBUG_ASSERT( po_start->offset <= po_end->offset );
	// cleanup
	zepto_response_to_request( target_mem_h );
	zepto_response_to_request( target_mem_h );
	// appending
	uint8_t* src_buff = memory_object_get_request_ptr( po_start->mem_handle ) + po_start->offset;
	ZEPTO_DEBUG_ASSERT( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, po_end->offset - po_start->offset );
	memcpy( dest_buff, src_buff, po_end->offset - po_start->offset );
}

void zepto_append_part_of_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end, MEMORY_HANDLE target_mem_h )
{
	ZEPTO_DEBUG_ASSERT( mem_h != target_mem_h );
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == mem_h );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == po_end->mem_handle );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	ZEPTO_DEBUG_ASSERT( po_start->offset <= po_end->offset );
	// copying
	uint8_t* src_buff = memory_object_get_request_ptr( po_start->mem_handle ) + po_start->offset;
	ZEPTO_DEBUG_ASSERT( src_buff != NULL );
	uint8_t* dest_buff = memory_object_append( target_mem_h, po_end->offset - po_start->offset );
	memcpy( dest_buff, src_buff, po_end->offset - po_start->offset );
}

/*
void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, uint16_t cutoff_cnt )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle == mem_h );
	ZEPTO_DEBUG_ASSERT( po_start->mem_handle < MEMORY_HANDLE_MAX ); 
	ZEPTO_DEBUG_ASSERT( po_start->offset + cutoff_cnt <= memory_object_get_request_size( mem_h ) );
	MEMORY_HANDLE ret_handle = po_start->mem_handle;
	po_start->mem_handle = MEMORY_HANDLE_INVALID;
	memory_object_cut_and_make_response( ret_handle, po_start->offset, memory_object_get_request_size( mem_h ) - cutoff_cnt - po_start->offset );
}
*/

void zepto_write_prepend_byte( MEMORY_HANDLE mem_h, uint8_t bt )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
	uint8_t b = bt;
//	memory_object_prepend( mem_h, &b, 1 );
	memory_object_prepend( mem_h, 1 );
	memcpy( memory_objects[ mem_h ].ptr, &b, 1 );
}

void zepto_write_prepend_block( MEMORY_HANDLE mem_h, const uint8_t* block, uint16_t size )
{
	ZEPTO_DEBUG_ASSERT( mem_h != MEMORY_HANDLE_INVALID );
//	memory_object_prepend( mem_h, block, size );
	memory_object_prepend( mem_h, size );
	memcpy( memory_objects[ mem_h ].ptr, block, size );
}



uint16_t zepto_writer_get_response_size( MEMORY_HANDLE mem_h )
{
	return memory_object_get_response_size( mem_h );
}

// inspired by SAGDP: creating a copy of the packet
/*
void zepto_writer_get_copy_of_response( MEMORY_HANDLE mem_h, uint8_t* buff )
{
	memcpy( buff, memory_object_get_response_ptr( mem_h ), memory_object_get_response_size( mem_h ) );
}
*/

#if (SA_USED_ENDIANNES == SA_LITTLE_ENDIAN)

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
	ZEPTO_DEBUG_ASSERT( 0 == "invalid subtraction" );
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
	ZEPTO_DEBUG_ASSERT( num_sz_max ); 
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

	uint8_t i;
	for ( i=1; i<8; i++ )
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

void zepto_parser_decode_uint_core( uint8_t** packed_num_bytes, uint8_t* bytes_out, uint8_t target_size )
{
	ZEPTO_DEBUG_ASSERT( target_size != 0 );
	ZEPTO_DEBUG_ASSERT( target_size <= 8 ); // TODO: implement and test for larger sizes
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

	uint8_t i;
	for ( i=1; i<8; i++ )
	{
		if ( (**packed_num_bytes & 0x80) == 0 )
		{
			if ( interm )
			{
				*bytes_out = (uint8_t)interm;
				bytes_out++;
			}
			ZEPTO_DEBUG_ASSERT( bytes_out - bytes_out_start <= target_size );
			(*packed_num_bytes)++;
			return;
		}
		ZEPTO_DEBUG_ASSERT( bytes_out - bytes_out_start <= target_size );
		(*packed_num_bytes)++;
//		interm += 128;
		interm += (1+(uint16_t)( **packed_num_bytes & 0x7F )) << (7-i);
		*bytes_out = (uint8_t)interm;
		bytes_out++;
		ZEPTO_DEBUG_ASSERT( bytes_out - bytes_out_start <= target_size );
		interm >>= 8;
	}
}

#elif (SA_USED_ENDIANNES == SA_BIG_ENDIAN)
// TODO: implement
#error not implemented; just do it
#else
#error SA_USED_ENDIANNES has unexpected value
#endif

void zepto_parser_decode_uint( parser_obj* po, uint8_t* bytes_out, uint8_t target_size )
{
	ZEPTO_DEBUG_ASSERT( po->mem_handle != MEMORY_HANDLE_INVALID );
	uint8_t* buff = memory_object_get_request_ptr( po->mem_handle ) + po->offset;
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	uint8_t* end = buff;
	zepto_parser_decode_uint_core( &end, bytes_out, target_size );
	ZEPTO_DEBUG_ASSERT( end - buff >= 0 && end - buff < 0x10000 ); // at least within 16 bits
	po->offset += (uint16_t)(end - buff);
}

uint16_t zepto_parse_encoded_uint16( parser_obj* po )
{
	uint16_t num_out;
	uint8_t buff[2];
	zepto_parser_decode_uint( po, buff, 2 );
	num_out = buff[1];
	num_out <<= 8;
	num_out |= buff[0];
	return num_out;
}


#if (SA_USED_ENDIANNES == SA_LITTLE_ENDIAN)

void zepto_parser_encode_and_append_uint( MEMORY_HANDLE mem_h, const uint8_t* num_bytes, uint8_t num_sz_max )
{
	ZEPTO_DEBUG_ASSERT( num_sz_max ); 
	ZEPTO_DEBUG_ASSERT( num_sz_max <= 16 ); // TODO: reason behind this limitation is fixed-size intermediate buffers declared below. If greater sizes desired, then either haave larger buffers (remember about stack size!), or implement a version that works locally over a few consequtive bytes
	uint8_t src_buff[16];
	uint8_t out_buff[16];
	uint8_t* out_buff_end = out_buff;
	while ( num_sz_max && ( num_bytes[ num_sz_max - 1 ] == 0 ) ) num_sz_max--;
	if ( num_sz_max == 0 )
	{
		*out_buff = 0;
		out_buff_end++;
	}
	else
	{
		memcpy( src_buff, num_bytes, num_sz_max );

		for(;;)
		{
			*out_buff_end = *src_buff & 0x7F;
			if ( is_less_128( src_buff, num_sz_max ) ) 
			{
				out_buff_end++;
				break;
			}
			*out_buff_end |= 128;
			out_buff_end++;
			shift_right_7( src_buff, num_sz_max );
			subtract_1( src_buff, num_sz_max );
		}
	}

	ZEPTO_DEBUG_ASSERT( out_buff_end - out_buff >= 0 && out_buff_end - out_buff < 0x100 ); // at least within 8 bits
	uint8_t sz = (uint8_t)(out_buff_end - out_buff);
	ZEPTO_DEBUG_PRINTF_3( "zepto_parser_encode_and_append_uint(..., ..., %d) resulted in %d bytes\n", num_sz_max, sz );
	uint8_t* buff = memory_object_append( mem_h, sz );
	memcpy( buff, out_buff, sz );
}

void zepto_parser_encode_and_append_uint16( MEMORY_HANDLE mem_h, uint16_t num )
{
	uint8_t buff[2];
	buff[0] = (uint8_t)num;
	buff[1] = (uint8_t)(num>>8);
	zepto_parser_encode_and_append_uint( mem_h, buff, 2 );
}

void zepto_parser_encode_and_prepend_uint( MEMORY_HANDLE mem_h, const uint8_t* num_bytes, uint8_t num_sz_max )
{
	ZEPTO_DEBUG_ASSERT( num_sz_max ); 
	ZEPTO_DEBUG_ASSERT( num_sz_max <= 16 ); // TODO: reason behind this limitation is fixed-size intermediate buffers declared below. If greater sizes desired, then either haave larger buffers (remember about stack size!), or implement a version that works locally over a few consequtive bytes
	uint8_t src_buff[16];
	uint8_t out_buff[16];
	uint8_t* out_buff_end = out_buff;
	while ( num_sz_max && ( num_bytes[ num_sz_max - 1 ] == 0 ) ) num_sz_max--;
	if ( num_sz_max == 0 )
	{
		*out_buff = 0;
		out_buff_end++;
	}
	else
	{
		memcpy( src_buff, num_bytes, num_sz_max );

		for(;;)
		{
			*out_buff_end = *src_buff & 0x7F;
			if ( is_less_128( src_buff, num_sz_max ) ) 
			{
				out_buff_end++;
				break;
			}
			*out_buff_end |= 128;
			out_buff_end++;
			shift_right_7( src_buff, num_sz_max );
			subtract_1( src_buff, num_sz_max );
		}
	}

	ZEPTO_DEBUG_ASSERT( out_buff_end - out_buff >= 0 && out_buff_end - out_buff < 0x100 ); // at least within 8 bits
	uint8_t sz = (uint8_t)(out_buff_end - out_buff);
//	ZEPTO_DEBUG_PRINTF_2( "zepto_parser_encode_and_prepend_uint(..., ..., %d) resulted in %d bytes\n", num_sz_max, sz );
//	memory_object_prepend( mem_h, out_buff, sz );
	memory_object_prepend( mem_h, sz );
	memcpy( memory_objects[ mem_h ].ptr, out_buff, sz );
}

void zepto_parser_encode_and_prepend_uint16( MEMORY_HANDLE mem_h, uint16_t num )
{
	uint8_t buff[2];
	buff[0] = (uint8_t)num;
	buff[1] = (uint8_t)(num>>8);
	zepto_parser_encode_and_prepend_uint( mem_h, buff, 2 );
}

#elif (SA_USED_ENDIANNES == SA_BIG_ENDIAN)
// TODO: implement
#error not yet implemented; just do it
#else
#error SA_USED_ENDIANNES has unexpected value
#endif

void zepto_parser_free_memory( REQUEST_REPLY_HANDLE mem_h )
{
	memory_object_free( mem_h );
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
