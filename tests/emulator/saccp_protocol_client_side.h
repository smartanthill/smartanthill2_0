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

#include "../../firmware/src/common/sa_common.h"
#include "../../firmware/src/common/sa_data_types.h"
#include "../../firmware/src/common/zepto_mem_mngmt.h"


// handlers
#ifdef MASTER_ENABLE_ALT_TEST_MODE

// RET codes
#define SACCP_RET_FAILED 0 // any failure
#define SACCP_RET_PASS_LOWER 1 // packet must be sent to a communication peer
#define SACCP_RET_CHAIN_DONE 2 // control program will be called start a new chain
#define SACCP_RET_CHAIN_CONTINUED 3 // control program will be called to prepare a subsequent packet

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id, void* control_prog_state );
//uint8_t saccp_control_program_process_incoming( parser_obj* po_start, parser_obj* po_end, void* control_prog_state );
//uint8_t saccp_control_program_prepare_new_program( MEMORY_HANDLE mem_h, void* control_prog_state );
//uint8_t handler_sacpp_send_new_program( MEMORY_HANDLE mem_h, void* control_prog_state );
uint8_t handler_sacpp_start_new_chain( MEMORY_HANDLE mem_h, void* control_prog_state );
uint8_t handler_sacpp_continue_chain( MEMORY_HANDLE mem_h, void* control_prog_state );

#else // MASTER_ENABLE_ALT_TEST_MODE

// RET codes
#define SACCP_RET_FAILED 0 // any failure
#define SACCP_RET_PASS_LOWER 1 // packet must be sent to a communication peer
#define SACCP_RET_PASS_TO_CENTRAL_UNIT 1 // packet must be sent to a communication peer

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h );
uint8_t handler_saccp_prepare_to_send( MEMORY_HANDLE mem_h );

#endif // MASTER_ENABLE_ALT_TEST_MODE


#endif // __SACCP_PROTOCOL_H__
