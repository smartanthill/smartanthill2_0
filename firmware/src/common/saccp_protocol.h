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


// handlers

// RET codes
#define SACCP_RET_FAILED 0 // any failure
#define SACCP_RET_PASS_LOWER 1 // packet must be sent to a communication peer

void zepto_vm_init();

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id );
//uint8_t handler_sacpp_reply( MEMORY_HANDLE mem_h );

#endif // __SACCP_PROTOCOL_H__
