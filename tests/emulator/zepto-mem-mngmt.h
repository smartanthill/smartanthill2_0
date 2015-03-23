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

#include "sa-common.h"

#define MEMORY_HANDLE uint8_t
#define REQUEST_REPLY_HANDLE MEMORY_HANDLE

struct parser_obj
{
	MEMORY_HANDLE mem_handle;
	uint16_t offset;
};


// parsing functions
void zepto_parser_init( parser_obj* po, REQUEST_REPLY_HANDLE mem_h );
uint8_t zepto_parse_uint8( parser_obj* po );
uint16_t zepto_parse_encoded_uint16( parser_obj* po );
void zepto_parse_read_block( parser_obj* po, uint8_t* block, uint16_t size );

// writing functions
void zepto_write_uint8( REQUEST_REPLY_HANDLE mem_h, uint8_t val );
void zepto_write_encoded_uint16( REQUEST_REPLY_HANDLE mem_h, uint16_t val );
void zepto_write_block( parser_obj* po, const uint8_t* block, uint16_t size );

#endif // __ZEPTO_MEM_MNGMT_H__