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

#if !defined __ZEPTO_MEM_MNGMT_HAL_SPEC_H__
#define __ZEPTO_MEM_MNGMT_HAL_SPEC_H__

#include "../zepto-mem-mngmt-base.h"

void memory_object_cut_and_make_response( REQUEST_REPLY_HANDLE mem_h, uint16_t offset, uint16_t size );
void memory_object_response_to_request( REQUEST_REPLY_HANDLE mem_h );
void memory_object_request_to_response( REQUEST_REPLY_HANDLE mem_h );
uint8_t* memory_object_append( REQUEST_REPLY_HANDLE mem_h, uint16_t size );
uint8_t* memory_object_prepend( REQUEST_REPLY_HANDLE mem_h, uint16_t size );
uint8_t* memory_object_get_request_ptr( REQUEST_REPLY_HANDLE mem_h );
uint16_t memory_object_get_request_size( REQUEST_REPLY_HANDLE mem_h );
uint8_t* memory_object_get_response_ptr( REQUEST_REPLY_HANDLE mem_h );
uint16_t memory_object_get_response_size( REQUEST_REPLY_HANDLE mem_h );


#endif // __ZEPTO_MEM_MNGMT_HAL_SPEC_H__