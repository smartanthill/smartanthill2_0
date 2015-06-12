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

/*******************************************************************************
THIS FILE IS MANUALLY OR AUTOMATICALLY GENERATED BASED ON DESIRED PLUGIN LIST
*******************************************************************************/


#include "sa_bodypart_list.h"

// include declarations of respective plugins
#include "plugins/smart-echo/smart-echo.h"

SmartEchoPluginConfig SmartEchoPluginConfig_struct =
{
	0,
};

SmartEchoPluginState SmartEchoPluginState_struct;


const bodypart_item bodyparts[ BODYPARTS_MAX ] ZEPTO_PROG_CONSTANT_LOCATION =
{
	{ smart_echo_plugin_handler_init, smart_echo_plugin_handler, &SmartEchoPluginConfig_struct, &SmartEchoPluginState_struct },
};
