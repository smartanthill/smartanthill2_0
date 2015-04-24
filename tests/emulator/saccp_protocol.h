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


#if !defined __SACCP_PROTOCOL_H__
#define __SACCP_PROTOCOL_H__

#include "sa-common.h"
#include "sa-data-types.h"
#include "zepto-mem-mngmt.h"


// RET codes
#define SACCP_RET_PASS_LOWER 0 // packet must be sent to a communication peer
#define SACCP_RET_IGNORE 1 // do nothing
#define SACCP_RET_FAILED 2 // any failure


// handlers
#ifdef USED_AS_MASTER

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id );
void saccp_control_program_process_incoming( parser_obj* po_start, parser_obj* po_end );
void saccp_control_program_prepare_new_program( MEMORY_HANDLE mem_h );
uint8_t handler_sacpp_send_new_program( MEMORY_HANDLE mem_h );

#else
uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id );
//uint8_t handler_sacpp_reply( MEMORY_HANDLE mem_h );
#endif

#endif // __SACCP_PROTOCOL_H__