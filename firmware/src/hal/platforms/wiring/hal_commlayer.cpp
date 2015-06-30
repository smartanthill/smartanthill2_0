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
#define BYTE_MARKER 0xFF

uint8_t hal_wait_for( waiting_for* wf )
{
    for (;;)
    {
        if (wf->wait_packet && Serial.available())
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

uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h )
{
    uint8_t buffer[ MAX_PACKET_SIZE ];

    uint8_t i, byte, marker_detected = 0;
    uint32_t timeout = 2000; // in ms
    uint32_t start_reading  = getTime();
    while ((start_reading + timeout) > getTime() && i < MAX_PACKET_SIZE)
    {
        if (!Serial.available())
            continue;

        byte = Serial.read();
        if (byte == BYTE_MARKER)
        {
            marker_detected = true;
            continue;
        }
        else if (byte == END_OF_TRANSMISSION)
        {
            ZEPTO_DEBUG_ASSERT( i && i <= MAX_PACKET_SIZE );
            zepto_write_block( mem_h, buffer, i );
            return HAL_GET_PACKET_BYTES_DONE;
        }
        else if (marker_detected)
        {
            uint8_t value = 0;
            switch (byte) {
                case 0x00:
                    value = 0x01;
                    break;
                case 0x02:
                    value = 0x17;
                    break;
                case 0x03:
                    value = 0xFF;
                    break;
                default:
                    return HAL_GET_PACKET_BYTES_FAILED;
            }
            buffer[i++] = value;
            marker_detected = false;
        }
        else
        {
            buffer[i++] = byte;
        }
    }

    return HAL_GET_PACKET_BYTES_FAILED;
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
    if (Serial.write (buff, sz) != sz)
        return COMMLAYER_RET_FAILED;
    else
        return COMMLAYER_RET_OK;
}
