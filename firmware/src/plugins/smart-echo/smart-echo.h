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


#if !defined __SA_TEST_PLUGINS_H__
#define __SA_TEST_PLUGINS_H__

#include "../../common/sa-common.h"
#include "../../common/sa-data-types.h"
#include "../../common/zepto-mem-mngmt.h"

#define PLUGIN_OK 0
#define PLUGIN_WAIT_TO_CONTINUE 1
#define PLUGIN_PASS_LOWER 2


typedef struct _SmartEchoPluginConfig //constant structure filled with a configuration for specific 'ant body part'
{
	uint8_t dummy;
/*	uint8_t bodypart_id;   //always present
	uint8_t request_pin_number;//pin to request sensor read
	uint8_t ack_pin_number;//pin to wait for to see when sensor has provided the data
	uint8_t reply_pin_numbers[4];//pins to read when ack_pin_number shows that the data is ready*/
} SmartEchoPluginConfig;

typedef struct _SmartEchoPluginState
{
	uint8_t state; //'0' means 'be ready to process incoming command', '1' means 'prepare reply'
	uint16_t last_sent_id;
	uint16_t currChainID[2];
	uint16_t currChainIdBase[2];
	uint8_t first_byte;
	uint16_t chain_id[2];
	uint16_t chain_ini_size;
	uint16_t reply_to_id;
	uint16_t self_id;
} SmartEchoPluginState;

uint8_t smart_echo_plugin_handler_init( const void* plugin_config, void* plugin_state );
uint8_t smart_echo_plugin_handler( const void* plugin_config, void* plugin_state, parser_obj* command, MEMORY_HANDLE reply/*, WaitingFor* waiting_for*/, uint8_t first_byte );


#endif // __SA_TEST_PLUGINS_H__