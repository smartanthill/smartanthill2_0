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

#if !defined SACCP_ON_COMM_STACK_SIDE

#ifdef MASTER_ENABLE_ALT_TEST_MODE


#include "../../firmware/src/common/sa_common.h"
#include "../../firmware/src/common/sa_data_types.h"
#include "../../firmware/src/common/zepto_mem_mngmt.h"
#include "../../firmware/src/common/saccp_protocol_constants.h"
#include "sa_test_control_prog.h"


// FRAME PARSER
#define FRAME_PARSER_OBJ_STATE_INI 0
#define FRAME_PARSER_OBJ_STATE_IN_FRAME 1
#define FRAME_PARSER_OBJ_STATE_DONE 2

#define FRAME_PARSER_OBJ_RET_NEW_EXCEPTION_FRAME 0
#define FRAME_PARSER_OBJ_RET_NEW_GOOD_FRAME 1
#define FRAME_PARSER_OBJ_RET_PARSING_DONE 2
#define FRAME_PARSER_OBJ_RET_UNEXPECTED_FRAME_TYPE 3

typedef struct _frame_parser_obj
{
	parser_obj po;
	uint8_t state;
} frame_parser_obj;

FORCE_INLINE
void frame_parser_obj_init( frame_parser_obj* fpo, const parser_obj* po_src )
{
	zepto_parser_init_by_parser( &(fpo->po), po_src );
	fpo->state = FRAME_PARSER_OBJ_STATE_INI;
}

FORCE_INLINE
uint8_t frame_parser_obj_start_frame( frame_parser_obj* fpo )
{
	if ( zepto_parsing_remaining_bytes( &(fpo->po) ) == 0 || fpo->state == FRAME_PARSER_OBJ_RET_PARSING_DONE )
	{
		fpo->state = FRAME_PARSER_OBJ_RET_PARSING_DONE;
		return FRAME_PARSER_OBJ_RET_PARSING_DONE;
	}
/*	parser_obj po1;
	zepto_parser_init_by_parser( &po1, &(fpo->po) );
	uint8_t first_byte = zepto_parse_uint8( &po1 );*/
	uint8_t first_byte = zepto_parse_uint8( &(fpo->po) );
	switch ( first_byte )
	{
		case FRAME_TYPE_DESCRIPTOR_REGULAR: return FRAME_PARSER_OBJ_RET_NEW_GOOD_FRAME; break;
		case FRAME_TYPE_DESCRIPTOR_EXCEPTION: return FRAME_PARSER_OBJ_RET_NEW_EXCEPTION_FRAME; break;
		default: ZEPTO_DEBUG_PRINTF_2( "Unexpected frame type %d\n", first_byte ); ZEPTO_DEBUG_ASSERT( 0 == "Unexpected frame type" ); return FRAME_PARSER_OBJ_RET_UNEXPECTED_FRAME_TYPE; break;
	}
}


// handlers

// RET codes
#define SACCP_RET_FAILED 0 // any failure
#define SACCP_RET_PASS_LOWER 1 // packet must be sent to a communication peer
#define SACCP_RET_CHAIN_DONE 2 // control program will be called start a new chain
#define SACCP_RET_CHAIN_CONTINUED 3 // control program will be called to prepare a subsequent packet

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, void* chain_id, DefaultTestingControlProgramState* state );

/*
uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h, sasp_nonce_type chain_id, void* control_prog_state );
//uint8_t saccp_control_program_process_incoming( parser_obj* po_start, parser_obj* po_end, void* control_prog_state );
//uint8_t saccp_control_program_prepare_new_program( MEMORY_HANDLE mem_h, void* control_prog_state );
//uint8_t handler_sacpp_send_new_program( MEMORY_HANDLE mem_h, void* control_prog_state );
uint8_t handler_sacpp_start_new_chain( MEMORY_HANDLE mem_h, void* control_prog_state );
uint8_t handler_sacpp_continue_chain( MEMORY_HANDLE mem_h, void* control_prog_state );
*/
#endif // MASTER_ENABLE_ALT_TEST_MODE

#else // SACCP_ON_COMM_STACK_SIDE

// RET codes
#define SACCP_RET_FAILED 0 // any failure
#define SACCP_RET_PASS_LOWER 1 // packet must be sent to a communication peer
#define SACCP_RET_PASS_TO_CENTRAL_UNIT 1 // packet must be sent to a communication peer

uint8_t handler_saccp_receive( MEMORY_HANDLE mem_h );
uint8_t handler_saccp_prepare_to_send( MEMORY_HANDLE mem_h );

#endif // SACCP_ON_COMM_STACK_SIDE

#endif // __SACCP_PROTOCOL_H__
