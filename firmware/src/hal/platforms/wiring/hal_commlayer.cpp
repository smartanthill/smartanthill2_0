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

#include "../../hal_commlayer.h"
#include "../../hal_waiting.h"

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
                return WAIT_RESULTED_IN_PACKET;
            }
        }
    }

    return WAIT_RESULTED_IN_FAILURE;
}

uint8_t wait_for_timeout( uint32_t timeout)
{
    ZEPTO_DEBUG_ASSERT(0);
    return 0;
}

uint8_t try_get_message( MEMORY_HANDLE mem_h )
{
    // do cleanup
    memory_object_response_to_request( mem_h );
    memory_object_response_to_request( mem_h );
    uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

    uint8_t i = 0, byte;
    while (Serial.available()) {
        byte = Serial.read();
        if (byte == END_OF_TRANSMISSION)
        {
            ZEPTO_DEBUG_ASSERT( i && i <= MAX_PACKET_SIZE );
            memory_object_response_to_request( mem_h );
            memory_object_cut_and_make_response( mem_h, 0, i );
            return COMMLAYER_RET_OK;
        }

        buff[i++] = byte;
    }

    return COMMLAYER_RET_FAILED;
}

bool communication_initialize()
{
    Serial.begin(9600);
    return true;
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
