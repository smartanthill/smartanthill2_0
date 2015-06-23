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

#if !defined __ZEPTO_MEM_MNGMT_H__
#define __ZEPTO_MEM_MNGMT_H__

#include "zepto_mem_mngmt_base.h"

//#define PLAIN_REPLY_FRAME	// TODO: this should be defined on a project level somewhere together with other system properties such as a level od Zepto-VM

// named memory handles
#define MEMORY_HANDLE_MAIN_LOOP 0
#define MEMORY_HANDLE_SAGDP_LSM 1
#define MEMORY_HANDLE_ADDITIONAL_ANSWER 2
#if !defined PLAIN_REPLY_FRAME
#define MEMORY_HANDLE_DEFAULT_PLUGIN 3
#ifdef _DEBUG
#define MEMORY_HANDLE_TEST_SUPPORT 4
#define MEMORY_HANDLE_DBG_TMP 5
#define MEMORY_HANDLE_MAX 6 // TODO: keep updated!!!
#else
#define MEMORY_HANDLE_MAX 4 // TODO: keep updated!!!
#endif
#else
#ifdef _DEBUG
#define MEMORY_HANDLE_TEST_SUPPORT 3
#define MEMORY_HANDLE_DBG_TMP 4
#define MEMORY_HANDLE_MAX 5 // TODO: keep updated!!!
#else
#define MEMORY_HANDLE_MAX 3 // TODO: keep updated!!!
#endif
#endif // PLAIN_REPLY_FRAME




typedef struct _parser_obj
{
	MEMORY_HANDLE mem_handle;
	uint16_t offset;
} parser_obj;

// UGLY HOOK FOR BY-PARTS (INITIAL PHASE OF) DEVELOPMENT
uint16_t ugly_hook_get_request_size( REQUEST_REPLY_HANDLE mem_h );
uint16_t ugly_hook_get_response_size( REQUEST_REPLY_HANDLE mem_h );
// end of UGLY HOOK FOR BY-PARTS (INITIAL PHASE OF) DEVELOPMENT



void zepto_mem_man_init_memory_management();


// parsing functions
void zepto_parser_init( parser_obj* po, REQUEST_REPLY_HANDLE mem_h );
void zepto_parser_init_by_parser( parser_obj* po, const parser_obj* po_base );
uint8_t zepto_parse_uint8( parser_obj* po );
bool zepto_parse_read_block( parser_obj* po, uint8_t* block, uint16_t size );
bool zepto_parse_skip_block( parser_obj* po, uint16_t size );
bool zepto_is_parsing_done( parser_obj* po );
uint16_t zepto_parsing_remaining_bytes( parser_obj* po );

// writing functions
void zepto_write_uint8( REQUEST_REPLY_HANDLE mem_h, uint8_t val );
//void zepto_write_encoded_uint16( REQUEST_REPLY_HANDLE mem_h, uint16_t val );
void zepto_write_block( REQUEST_REPLY_HANDLE mem_h, const uint8_t* block, uint16_t size );

// extended writing functions
void zepto_response_to_request( MEMORY_HANDLE mem_h );
void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end );
void zepto_copy_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h );
void zepto_copy_response_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h );
void zepto_append_response_to_response_of_another_handle( MEMORY_HANDLE mem_h, MEMORY_HANDLE target_mem_h );
void zepto_copy_part_of_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end, MEMORY_HANDLE target_mem_h );
void zepto_append_part_of_request_to_response_of_another_handle( MEMORY_HANDLE mem_h, parser_obj* po_start, parser_obj* po_end, MEMORY_HANDLE target_mem_h );
//void zepto_convert_part_of_request_to_response( MEMORY_HANDLE mem_h, parser_obj* po_start, uint16_t cutoff_cnt )
void zepto_write_prepend_byte( MEMORY_HANDLE mem_h, uint8_t bt );
void zepto_write_prepend_block( MEMORY_HANDLE mem_h, const uint8_t* block, uint16_t size );

uint16_t zepto_writer_get_response_size( MEMORY_HANDLE mem_h );

void zepto_parser_free_memory( REQUEST_REPLY_HANDLE mem_h );
void zepto_parser_free_response( REQUEST_REPLY_HANDLE mem_h );
void zepto_parser_strip_beginning_of_request( parser_obj* po );
/*
void zepto_writer_get_copy_of_response( MEMORY_HANDLE mem_h, uint8_t* buff );
*/

// specific encoded uint functions
//void zepto_parser_encode_uint( const uint8_t* num_bytes, uint8_t num_sz_max, uint8_t** bytes_out );
//void zepto_parser_decode_uint( const uint8_t** packed_num_bytes, uint8_t* bytes_out, uint8_t target_size );
//uint16_t zepto_parse_encoded_uint16( parser_obj* po );
uint16_t zepto_parse_encoded_uint16( parser_obj* po );
void zepto_parser_decode_uint( parser_obj* po, uint8_t* bytes_out, uint8_t target_size );

void zepto_parser_encode_and_append_uint16( MEMORY_HANDLE mem_h, uint16_t num );
void zepto_parser_encode_and_append_uint( MEMORY_HANDLE mem_h, const uint8_t* num_bytes, uint8_t num_sz_max );
void zepto_parser_encode_and_prepend_uint16( MEMORY_HANDLE mem_h, uint16_t num );
void zepto_parser_encode_and_prepend_uint( MEMORY_HANDLE mem_h, const uint8_t* num_bytes, uint8_t num_sz_max );
#endif // __ZEPTO_MEM_MNGMT_H__