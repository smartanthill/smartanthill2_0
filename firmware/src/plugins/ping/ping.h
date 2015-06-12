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


#if !defined __SA_PING_PLUGIN_H__
#define __SA_PING_PLUGIN_H__

#include "../../common/sa-common.h"
#include "../../common/sa-data-types.h"

typedef struct _PingPluginConfig
{
	uint8_t dummy;
} PingPluginConfig;

typedef struct _PingPluginState
{
	uint8_t dummy;
} PingPluginState;

uint8_t ping_plugin_handler_init( const void* plugin_config, void* plugin_state );
uint8_t ping_plugin_handler( const void* plugin_config, void* plugin_state, parser_obj* command, MEMORY_HANDLE reply/*, WaitingFor* waiting_for*/, uint8_t first_byte );


#endif // __SA_PING_PLUGIN_H__