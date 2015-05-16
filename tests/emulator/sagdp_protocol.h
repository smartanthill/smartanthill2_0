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


#if !defined __SAGDP_PROTOCOL_H__
#define __SAGDP_PROTOCOL_H__

#include "sa-common.h"
//#include "sa-eeprom.h"
#include "zepto-mem-mngmt.h"
#include "sa-data-types.h"
#include "hal/sa-hal-time-provider.h"


// RET codes
#define SAGDP_RET_SYS_CORRUPTED 0 // data processing inconsistency detected
#define SAGDP_RET_OK 1 // no output is available and no further action is required (for instance, after getting PID)
#define SAGDP_RET_TO_LOWER_NEW 2 // new packet
#define SAGDP_RET_TO_LOWER_REPEATED 3 // repeated packet
#define SAGDP_RET_TO_LOWER_NONE 4 // repeated packet
#define SAGDP_RET_TO_HIGHER 5
//#define SAGDP_RET_TO_HIGHER_ERROR 6
#define SAGDP_RET_START_OVER_FIRST_RECEIVED 7 // "first" packet is received while SAGDP is in "wait-remote" state
#define SAGDP_RET_NEED_NONCE 8 // nonce is necessary before sending a packet


// SAGDP States
#define SAGDP_STATE_NOT_INITIALIZED 0
#define SAGDP_STATE_IDLE 0 // TODO: implement transition from non_initialized to idle state or ensure there is no difference in states
#define SAGDP_STATE_WAIT_REMOTE 2
#define SAGDP_STATE_WAIT_LOCAL 3

// time-related events
#define SAGDP_EV_NONE 0
#define SAGDP_EV_RESEND_LSP 1


// packet statuses within the chain
#define SAGDP_P_STATUS_INTERMEDIATE 0
#define SAGDP_P_STATUS_FIRST 1
#define SAGDP_P_STATUS_TERMINATING 2
#define SAGDP_P_STATUS_MASK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING )
#define SAGDP_P_STATUS_NO_RESEND 4
#define SAGDP_P_STATUS_ACK 8
#define SAGDP_P_STATUS_ERROR_MSG ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING )
#define SAGDP_P_STATUS_IS_ACK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING | SAGDP_P_STATUS_ACK )
#define SAGDP_P_STATUS_FULL_MASK ( SAGDP_P_STATUS_FIRST | SAGDP_P_STATUS_TERMINATING | SAGDP_P_STATUS_NO_RESEND | SAGDP_P_STATUS_ACK )


// sagdp data structure
typedef struct _SAGDP_DATA
{
	uint8_t state;
	uint8_t last_timeout;
	uint8_t resent_ordinal;
	sasp_nonce_type last_received_chain_id;
	sasp_nonce_type last_received_packet_id;
	sasp_nonce_type first_last_sent_packet_id;
	sasp_nonce_type next_last_sent_packet_id;
	sasp_nonce_type prev_first_last_sent_packet_id;
	sa_time_val next_event_time;
	uint8_t event_type; // one of time-related events
} SAGDP_DATA;


// handlers
void sagdp_init( SAGDP_DATA* sagdp_data );
uint8_t handler_sagdp_timer( timeout_action* tact, sasp_nonce_type nonce, REQUEST_REPLY_HANDLE mem_h, SAGDP_DATA* sagdp_data );
uint8_t handler_sagdp_receive_up( timeout_action* tact, sasp_nonce_type nonce, uint8_t* pid, REQUEST_REPLY_HANDLE mem_h, SAGDP_DATA* sagdp_data );
uint8_t handler_sagdp_receive_request_resend_lsp( timeout_action* tact, sasp_nonce_type nonce, MEMORY_HANDLE mem_h, SAGDP_DATA* sagdp_data );
uint8_t handler_sagdp_receive_hlp( timeout_action* tact, sasp_nonce_type nonce, MEMORY_HANDLE mem_h, SAGDP_DATA* sagdp_data );

#endif // __SAGDP_PROTOCOL_H__