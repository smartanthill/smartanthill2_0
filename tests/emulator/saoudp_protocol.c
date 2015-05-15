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


#include "saoudp_protocol.h"

#define SAOIP_HEADER_INTERNAL 1
#define SAOIP_HEADER_AGGREGATE_REQUEST 2
#define SAOIP_HEADER_AGGREGATE_REPLY 3
#define SAOIP_HEADER_LAST_HEADER 4

uint8_t handler_saoudp_send( MEMORY_HANDLE mem_h )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	PRINTF( "Entering handler_saoudp_receive(); msg. sz = %d\n", zepto_parsing_remaining_bytes( &po ) );

	zepto_parser_init_by_parser( &po1, &po );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );

	// we willl add only a minimally required Last Header
	// TODO: consider other types of headers

	zepto_parser_encode_and_prepend_uint16( mem_h, SAOIP_HEADER_LAST_HEADER );

	return SAOUDP_RET_OK;
}

uint8_t handler_saoudp_receive( MEMORY_HANDLE mem_h )
{
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	PRINTF( "Entering handler_saoudp_receive(); msg. sz = %d\n", zepto_parsing_remaining_bytes( &po ) );

	// read optional headers
	bool more_hdrs = true;
	do
	{
		uint16_t header_type = zepto_parse_encoded_uint16( &po );
		PRINTF( "handler_saoudp_receive(): header type = %d\n", header_type );
		switch ( header_type )
		{
			case SAOIP_HEADER_INTERNAL:
			case SAOIP_HEADER_AGGREGATE_REQUEST:
			case SAOIP_HEADER_AGGREGATE_REPLY:
			{
				PRINTF( "processing of header type %d is not yet implemented\n", header_type );
				assert( 0 );
				break;
			}
			case SAOIP_HEADER_LAST_HEADER:
			{
				more_hdrs = false;
				break;
			}
			default:
			{
				PRINTF( "Unknown header type %d\n", header_type );
				assert( 0 );
				break;
			}
		}
	}
	while ( more_hdrs );
	zepto_parser_init_by_parser( &po1, &po );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
	return SAOUDP_RET_OK;
}


