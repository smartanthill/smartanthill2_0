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

#if defined ARDUINO && (!defined ENERGIA)

#include "../../sa-commlayer.h"
#include "../../hal-waiting.h"

#define MAX_PACKET_SIZE 50
#define START_OF_PACKET 0x1
#define END_OF_TRANSMISSION 0x17

uint8_t hal_wait_for( waiting_for* wf )
{
    if (wf->wait_packet != 0)
    {
        while (Serial.available())
        {
            if (Serial.read() == START_OF_PACKET)
            {
                return WAIT_RESULTED_IN_PACKET
            }
        }
    }

	return WAIT_RESULTED_IN_FAILURE;
}

uint8_t try_get_message( MEMORY_HANDLE mem_h )
{
    // do cleanup
    memory_object_response_to_request( mem_h );
    memory_object_response_to_request( mem_h );
    uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

    uint8_t index = 0, byte;
    while (Serial.available()) {
        byte = Serial.read();
        if (byte == END_OF_TRANSMISSION)
        {
            return COMMLAYER_RET_OK;
        }

        buff[index++] = byte;
    }

    return COMMLAYER_RET_FAILED;
}

bool communication_initialize()
{
	ZEPTO_DEBUG_ASSERT(0);
	return false;
}

uint8_t send_message( MEMORY_HANDLE mem_h )
{
    uint16_t sz = memory_object_get_request_size( mem_h );
    ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
    uint8_t* buff = memory_object_get_request_ptr( mem_h );
    ZEPTO_DEBUG_ASSERT( buff != NULL );
    Serial.write(buff, sz);
    return COMMLAYER_RET_OK;
}

#endif
