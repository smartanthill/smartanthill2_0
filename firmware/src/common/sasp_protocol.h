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

#if !defined __SASP_PROTOCOL_H__
#define __SASP_PROTOCOL_H__

#include "sa-common.h"
#include "sa-data-types.h"
#include "sa-eeprom.h"
#include "zepto-mem-mngmt.h"

// RET codes
#define SASP_RET_IGNORE 0 // not authenticated, etc
#define SASP_RET_TO_HIGHER_NEW 1 // new packet
#define SASP_RET_TO_HIGHER_LAST_SEND_FAILED 3 // sending of last packet failed (for instance, old nonce)
#define SASP_RET_TO_LOWER_REGULAR 4 // for regular sending
#define SASP_RET_TO_LOWER_ERROR 5 // for error messaging
#define SASP_RET_NONCE 6 // buffer out contains nonce


// sizes
#define SASP_NONCE_SIZE 6
#define SASP_HEADER_SIZE SASP_NONCE_SIZE
#define SASP_ENC_BLOCK_SIZE 16
#define SASP_TAG_SIZE SASP_ENC_BLOCK_SIZE


// data structures
typedef struct _SASP_DATA
{
	sasp_nonce_type nonce_lw;
	sasp_nonce_type nonce_ls;
} SASP_DATA;
//#define DATA_SASP_SIZE (SASP_NONCE_SIZE+SASP_NONCE_SIZE+SASP_TAG_SIZE)
//#define DATA_SASP_NONCE_LW_OFFSET 0 // Nonce Lower Watermark
//#define DATA_SASP_NONCE_LS_OFFSET SASP_NONCE_SIZE // Nonce to use For Sending
//#define DATA_SASP_LRPS_OFFSET (SASP_NONCE_SIZE+SASP_NONCE_SIZE) // Last Received Packet Signature TODO: Check whether we need it


// initializing and backup
void sasp_init_at_lifestart( /*SASP_DATA* sasp_data*/ );
void sasp_restore_from_backup( /*SASP_DATA* sasp_data*/ );

// handlers
uint8_t handler_sasp_receive( const uint8_t* key, uint8_t* packet_id, MEMORY_HANDLE mem_h/*, SASP_DATA* sasp_data*/ );
uint8_t handler_sasp_send( const uint8_t* key, const uint8_t* packet_id, MEMORY_HANDLE mem_h/*, SASP_DATA* sasp_data*/ );
uint8_t handler_sasp_get_packet_id( uint8_t* buffOut, int buffOutSize/*, SASP_DATA* sasp_data*/ );
uint8_t handler_sasp_save_state( /*SASP_DATA* sasp_data*/ );


#endif // __SASP_PROTOCOL_H__