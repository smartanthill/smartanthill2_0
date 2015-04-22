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


#include "sasp_protocol.h"
#include "zepto-mem-mngmt.h"
#include "sa-eax-128.h"

//#define NEW_APPROACH



void SASP_initAtLifeStart( uint8_t* dataBuff )
{
	memset( dataBuff + DATA_SASP_NONCE_LW_OFFSET, 0, SASP_NONCE_SIZE );
	memset( dataBuff + DATA_SASP_NONCE_LS_OFFSET, 0, SASP_NONCE_SIZE );
//	*( dataBuff + DATA_SASP_NONCE_LS_OFFSET + SASP_NONCE_SIZE - 1 ) = 1;
	*( dataBuff + DATA_SASP_NONCE_LS_OFFSET ) = 1;
//	memset( dataBuff + DATA_SASP_LRPS_OFFSET, 0, SASP_TAG_SIZE );

	eeprom_write( DATA_SASP_NONCE_LW_ID, dataBuff + DATA_SASP_NONCE_LW_OFFSET, SASP_NONCE_SIZE );
	eeprom_write( DATA_SASP_NONCE_LW_ID, dataBuff + DATA_SASP_NONCE_LW_OFFSET, SASP_NONCE_SIZE );
}

void SASP_restoreFromBackup( uint8_t* dataBuff )
{
//	memset( dataBuff + DATA_SASP_LRPS_OFFSET, 0, SASP_TAG_SIZE );

	uint8_t size;

	size = eeprom_read_size( DATA_SASP_NONCE_LW_ID );
	assert( size == SASP_NONCE_SIZE );
	eeprom_read_fixed_size( DATA_SASP_NONCE_LW_ID, dataBuff + DATA_SASP_NONCE_LW_OFFSET, size);

	size = eeprom_read_size( DATA_SASP_NONCE_LS_ID );
	assert( size == SASP_NONCE_SIZE );
	eeprom_read_fixed_size( DATA_SASP_NONCE_LS_ID, dataBuff + DATA_SASP_NONCE_LS_OFFSET, size);
}

void SASP_NonceLS_increment(  uint8_t* nonce )
{
	int8_t i;
	for ( i=0; i<SASP_NONCE_SIZE; i++ )
	{
		nonce[i] ++;
		if ( nonce[i] ) break;
	}
}

int8_t SASP_NonceCompare( const uint8_t* nonce1, const uint8_t* nonce2 )
{
	int8_t i;
	for ( i=SASP_NONCE_SIZE-1; i>=0; i-- )
	{
		if ( nonce1[i] > nonce2[i] ) return int8_t(1);
		if ( nonce1[i] < nonce2[i] ) return int8_t(-1);
	}
	return 0;
}
/*
bool SASP_NonceIsIntendedForSasp(  const uint8_t* nonce )
{
	return nonce[SASP_NONCE_SIZE-1] & ((uint8_t)0x80);
}

void SASP_NonceSetIntendedForSaspFlag(  uint8_t* nonce )
{
	nonce[SASP_NONCE_SIZE-1] |= 0x80;
}

void SASP_NonceClearForSaspFlag(  uint8_t* nonce )
{
	nonce[SASP_NONCE_SIZE-1] &= 0x7F;
}
*/
void SASP_EncryptAndAddAuthenticationData( REQUEST_REPLY_HANDLE mem_h/*, uint8_t* _stack, int stackSize*/, const uint8_t* key, const uint8_t* packet_id )
{
	// msgSize covers the size of message_byte_sequence
	// (byte with MASTER_SLAVE_BIT) | nonce concatenation is used as a full nonce
	// the output is formed as follows: prefix (with nonce; not a EAX header) | encrypted data | tag

	// data under encryption is as follows: first_byte | message_byte_sequence

	bool read_ok;

	uint8_t nonce[7];
	memcpy( nonce, packet_id, 6 );
	nonce[6] = MASTER_SLAVE_BIT;

	// intermediate buffers for EAX
	uint8_t tag[SASP_TAG_SIZE];

	// init parser object
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );

	// init stack
//	uint8_t* stack_start = _stack;
//	uint8_t* stack = _stack;

	// read first byte
/*	uint8_t first_byte = zepto_parse_uint8( &po );
	uint16_t msg_size = zepto_parsing_remaining_bytes( &po );

	zepto_parser_init( &po1, &po );
	zepto_parse_skip_block( &po1, msg_size );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );

	uint16_t padding_size = SASP_ENC_BLOCK_SIZE - ( msg_size + 1 ) % SASP_ENC_BLOCK_SIZE;

	if ( padding_size )
	{
		first_byte |= 0x80; // TODO: use bit-field processing instead
		zepto_write_prepend_byte( mem_h, padding_size );
		zepto_write_prepend_byte( mem_h, first_byte );
		zepto_write_block( mem_h, block, padding_size - 1 ); // TODO 1: check sizes; TODO 2: it's a good place to add some fancy padding (say, all zeros, or rand generated)
	}
	else
	{
		zepto_write_prepend_byte( mem_h, first_byte );
	}
	zepto_write_prepend_byte( mem_h, first_byte );

	zepto_response_to_request( mem_h );
	zepto_parser_init( &po, mem_h );*/

	// encrypt and generate authentication data
/*	while ( zepto_parsing_remaining_bytes( &po ) )
	{
		bool read_ok = zepto_parse_read_block( &po, stack, SASP_ENC_BLOCK_SIZE );
		if ( !read_ok )
		{
			assert( 0 == "block size is not multiple of SASP_ENC_BLOCK_SIZE" );
			return; // TODO: process!!!
		}

		// do DUMMY authentication/decryption
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			stack[ i ] ^= 'y'; // dummy encrypt
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			c ^= stack[ i ]; // toward dummy tag

		// write decrypted block to output
		zepto_write_block( mem_h, stack, SASP_ENC_BLOCK_SIZE );
	}

	// form DUMMY tag
	for ( i=0; i<SASP_TAG_SIZE; i++)
		stack[i] = c;*/
	if ( zepto_parsing_remaining_bytes( &po ) )
	{
		uint8_t block[16];
		uint8_t ctr[16];
		uint8_t msg_cbc_val[16];
		eax_128_init_for_nonzero_msg( key, nonce, SASP_NONCE_SIZE, ctr, msg_cbc_val, tag );

		// process blocks
		// in actual implementation using the same logic it's a place to write resulting packet encrypted part
		while ( zepto_parsing_remaining_bytes( &po ) > 16 )
		{
			read_ok = zepto_parse_read_block( &po, block, SASP_ENC_BLOCK_SIZE );
			assert( read_ok );
			eax_128_process_nonterminating_block_encr( key, ctr, block, block, msg_cbc_val );
			zepto_write_block( mem_h, block, SASP_ENC_BLOCK_SIZE );
		}
		uint8_t remaining_sz = zepto_parsing_remaining_bytes( &po );
		read_ok = zepto_parse_read_block( &po, block, remaining_sz );
		assert( read_ok );
		eax_128_process_terminating_block_encr( key, ctr, block, remaining_sz, block, msg_cbc_val );
		zepto_write_block( mem_h, block, remaining_sz );

		// finalize
		eax_128_finalize_for_nonzero_msg( key, NULL, 0, ctr, msg_cbc_val, tag, SASP_TAG_SIZE );
	}
	else // zero length is a special case
	{
		eax_128_calc_tag_of_zero_msg( key, NULL, 0, nonce, SASP_NONCE_SIZE, tag, SASP_TAG_SIZE );
	}


	// uppend tag, and init and prepend prefix
	zepto_write_block( mem_h, tag, SASP_TAG_SIZE );

/*	uint8_t* prefix = nonce; // reuse memory
	uint8_t* pr_end = prefix;
	zepto_parser_encode_uint( packet_id, SASP_NONCE_SIZE, &pr_end );
	zepto_write_prepend_block( mem_h, prefix, pr_end - prefix );*/
	zepto_parser_encode_and_prepend_uint( mem_h, packet_id, SASP_NONCE_SIZE );

//	zepto_write_prepend_block( mem_h, nonce, SASP_NONCE_SIZE );
/*	uint8_t* block_end = block;
	zepto_parser_encode_uint( block, SASP_NONCE_SIZE, &block_end );
	zepto_write_prepend_block( mem_h, block, block_end - block );*/
}

bool SASP_IntraPacketAuthenticateAndDecrypt( const uint8_t* key, REQUEST_REPLY_HANDLE mem_h, uint8_t* pid ) 
{
	// input data structure: prefix (with nonce; not a part of EAX header) | encrypted data | tag
	// Structure of output buffer: first_byte | byte_sequence
	// pid (Packet ID) is formed from prefix data
	
	
	// DUMMY values used only until actual authentication/decryption is implemented
/*	uint8_t nonce[ SASP_NONCE_SIZE ];
	uint8_t header[ SASP_HEADER_SIZE ];*/
	uint8_t block[ SASP_ENC_BLOCK_SIZE ];
	uint8_t tag_calculted[16];
	uint8_t ctr[16];
	uint8_t msg_cbc_val[16];
//	uint8_t* header_prim = ctr; // we will reuse it when ctr is no longer needed

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	// init stack
/*	uint8_t* stack_start = _stack;
	uint8_t* stack = _stack;*/

	// read and form PID
	uint8_t nonce[ 7 ];
	zepto_parser_decode_uint( &po, nonce, 6 );
	memcpy( pid, nonce, 6 );
	nonce[6] = 1 - MASTER_SLAVE_BIT;
/*	pid[ SASP_NONCE_SIZE - 1] &= 0x7F; // remove SASP bit; TODO: potentially, we need to use bit-fiels processing instead
	uint8_t nonce[ SASP_NONCE_SIZE ];
	memcpy( nonce, pid, SASP_HEADER_SIZE );
	nonce[ SASP_NONCE_SIZE - 1] &= ( 1 - MASTER_SLAVE_BIT ) << 7; // add sending sidde bit; TODO: potentially, we need to use bit-fiels processing instead
*/
	// read blocks one by one, authenticate, decrypt, write
	bool read_ok;
	if ( zepto_parsing_remaining_bytes( &po ) > SASP_TAG_SIZE )
	{
		eax_128_init_for_nonzero_msg( key, nonce, SASP_NONCE_SIZE, ctr, msg_cbc_val, tag_calculted );
		while ( zepto_parsing_remaining_bytes( &po ) > SASP_TAG_SIZE + SASP_ENC_BLOCK_SIZE )
		{
			read_ok = zepto_parse_read_block( &po, block, SASP_ENC_BLOCK_SIZE );
			if ( !read_ok )
				return false; // if any bytes, then the whole block

			// do DUMMY authentication/decryption
	/*		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] ^= 'y'; // dummy decrypt
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i ]; // toward dummy tag*/

			eax_128_process_nonterminating_block_decr( key, ctr, block, block, msg_cbc_val );
			// write decrypted block to output
			zepto_write_block( mem_h, block, SASP_ENC_BLOCK_SIZE );
		}
		// process terminating block ( 1-16 bytes)
		uint8_t remaining_sz = zepto_parsing_remaining_bytes( &po ) - SASP_TAG_SIZE;
		read_ok = zepto_parse_read_block( &po, block, remaining_sz );
		if ( !read_ok )
			return false; // if any bytes, then the whole block
		eax_128_process_terminating_block_decr( key, ctr, block, remaining_sz, block, msg_cbc_val );
		zepto_write_block( mem_h, block, remaining_sz );
		eax_128_finalize_for_nonzero_msg( key, NULL, 0, ctr, msg_cbc_val, tag_calculted, SASP_TAG_SIZE );
	}
	else
	{
		eax_128_calc_tag_of_zero_msg( key, NULL, 0, nonce, SASP_NONCE_SIZE, tag_calculted, SASP_TAG_SIZE );
	}
	read_ok = zepto_parse_read_block( &po, block, SASP_TAG_SIZE );
	if ( !read_ok )
		return false; // if any bytes, then the whole block

	// now we have a decrypted data as output, and a tag "calculated"; check the tag
	for ( uint8_t i=0; i<SASP_TAG_SIZE; i++ )
		if ( tag_calculted[i] != block[i] ) return false;

	// decrease stack pointer to point again to tag

	// read tag; note that we do not need it until the end of authentication, so, in general, it could be read later (if proper parser calls are available)
/*	read_ok = zepto_parse_read_block( &po, stack, SASP_ENC_BLOCK_SIZE );
	zepto_parse_read_block( &po, stack, SASP_TAG_SIZE );
	if ( !read_ok )
		return false; // if any bytes, then the whole block
//	stack += SASP_TAG_SIZE;
//	stack -= SASP_TAG_SIZE;

	// do DUMMY authentication
	bool tag_ok = true;
	for ( i=0; i<SASP_TAG_SIZE; i++)
		tag_ok = tag_ok && stack[i] == c;

	if ( !tag_ok )
		return false;*/

/*	zepto_response_to_request( mem_h );
	zepto_parser_init( &po, mem_h );

	uint8_t first_byte = zepto_parse_uint8( &po );

	uint16_t padding_size = 0;
	if ( first_byte & 0x80 ) // TODO: use bit-fiels processing instead
		padding_size = zepto_parse_uint8( &po ) - 1; // TODO: NOTE!!!! in the new implementation this MAY be revised
	uint16_t body_size = zepto_parsing_remaining_bytes( &po ) - padding_size;
	uint16_t body_size = zepto_parsing_remaining_bytes( &po );

	parser_obj po1;
	zepto_parser_init( &po1, &po );
	zepto_parse_skip_block( &po1, body_size );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
	first_byte &= 0x7; // TODO: use bit-fiels processing instead
	zepto_write_prepend_byte( mem_h, first_byte );*/

	return true;
}


bool SASP_is_for_sasp( REQUEST_REPLY_HANDLE mem_h ) 
{
	parser_obj po;
	zepto_response_to_request( mem_h );
	zepto_parser_init( &po, mem_h );
	uint8_t first_byte = zepto_parse_uint8( &po );
	bool for_sasp = first_byte & 0x80; // TODO: use bit field processing instead

	// NOTE: we do not need to clear 'is-for-sasp' flag since, if set, it will be processed by sasp handler itself (no confusion on upper levels)

	zepto_parser_init( &po, mem_h );
	parser_obj po1;
	zepto_parser_init( &po1, &po );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );

	return for_sasp;
}

void DEBUG_SASP_EncryptAndAddAuthenticationDataChecked( MEMORY_HANDLE mem_h/**/, const uint8_t* key, const uint8_t* nonce )
{
	uint8_t header[6];

	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );
	uint16_t inisz = zepto_parsing_remaining_bytes( &po );

	PRINTF( "handler_sasp_send(): nonce used: %02x %02x %02x %02x %02x %02x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );
	PRINTF( "handler_sasp_send(): size: %d\n", inisz );
/*	if ( inisz == 0 )
	{
		inisz = 0;
	}*/

	uint16_t encr_sz, decr_sz;
	uint8_t inimsg[1024]; zepto_parse_read_block( &po, inimsg, inisz );
	uint8_t checkedMsg[1024];
//	uint8_t dbg_stack[1024];
	uint8_t dbg_nonce[6];


	// do required job
	SASP_EncryptAndAddAuthenticationData( mem_h/*, stack, stackSize*/, key, nonce );

	// CHECK RESULTS

	// init parser object
	zepto_parser_init( &po, mem_h );
	zepto_parser_init( &po1, mem_h );	


	// TODO: if this debug code remains in use, lines below must be replaced by getting a new handle
	uint8_t /*x_buff1_x[1024],*/ x_buff1_x2[1024];
/*	REQUEST_REPLY_HANDLE mem_h2 = 110;
	uint8_t* x_buff1 = x_buff1_x + 512;
	memory_objects[ mem_h2 ].ptr = x_buff1;
	memory_objects[ mem_h2 ].rq_size = 0;
	memory_objects[ mem_h2 ].rsp_size = 0;*/
	// copy output to input of a new handle and restore
	zepto_response_to_request( mem_h );
	encr_sz = zepto_parsing_remaining_bytes( &po1 );
	decr_sz = encr_sz;
	zepto_parse_read_block( &po1, x_buff1_x2, encr_sz );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 ); // restore the picture
	zepto_response_to_request( MEMORY_HANDLE_DBG_TMP );
	zepto_write_block( MEMORY_HANDLE_DBG_TMP, x_buff1_x2, encr_sz );
	zepto_response_to_request( MEMORY_HANDLE_DBG_TMP );

	bool ipaad = SASP_IntraPacketAuthenticateAndDecrypt( key, MEMORY_HANDLE_DBG_TMP, dbg_nonce );
	assert( ipaad );
	zepto_response_to_request( MEMORY_HANDLE_DBG_TMP );
	zepto_parser_init( &po, MEMORY_HANDLE_DBG_TMP );
	decr_sz = zepto_parsing_remaining_bytes( &po );
	zepto_parse_read_block( &po, checkedMsg, decr_sz );

	PRINTF( "handler_sasp_send():     nonce: %02x %02x %02x %02x %02x %02x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );
	PRINTF( "handler_sasp_send(): dbg_nonce: %02x %02x %02x %02x %02x %02x\n", dbg_nonce[0], dbg_nonce[1], dbg_nonce[2], dbg_nonce[3], dbg_nonce[4], dbg_nonce[5] );
	assert( ipaad );
	assert( decr_sz == inisz );
	assert( memcmp( nonce, dbg_nonce, 6 ) == 0 );
//	checkedMsg[0] &= 0x7F; // get rid of SASP bit
	for ( int k=0; k<decr_sz; k++ )
		assert( inimsg[k] == checkedMsg[k] );
}

uint8_t handler_sasp_send( const uint8_t* key, const uint8_t* packet_id, MEMORY_HANDLE mem_h, uint8_t* data )
{
//	, header[6];

	assert( SASP_NonceCompare( packet_id, data + DATA_SASP_NONCE_LS_OFFSET ) >= 0 );

//	SASP_EncryptAndAddAuthenticationData( mem_h/*, stack, stackSize*/, packet_id );
	DEBUG_SASP_EncryptAndAddAuthenticationDataChecked( mem_h/*, stack, stackSize*/, key, packet_id );

	INCREMENT_COUNTER( 10, "handler_sasp_send()" );
	return SASP_RET_TO_LOWER_REGULAR;
}

uint8_t handler_sasp_receive( const uint8_t* key, uint8_t* pid, MEMORY_HANDLE mem_h, uint8_t* data )
{
//	uint8_t header[ SASP_HEADER_SIZE ];

	INCREMENT_COUNTER( 11, "handler_sasp_receive()" );
	// 1. Perform intra-packet authentication
	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	uint16_t packet_size = zepto_parsing_remaining_bytes( &po );
	if ( packet_size == 0 )
	{
		return SASP_RET_IGNORE;
	}

//	zepto_parse_read_block( &po, stack, SASP_NONCE_SIZE );
/*	zepto_parser_decode_uint( &po, header, SASP_HEADER_SIZE );
	bool for_sasp = SASP_NonceIsIntendedForSasp( header );
	uint16_t debug_rem_b = zepto_parsing_remaining_bytes( &po );
	if ( for_sasp )
	{
		PRINTF( "handler_sasp_receive(): for sasp; nonce: %02x %02x %02x %02x %02x %02x\n", header[0], header[1], header[2], header[3], header[4], header[5] );
		PRINTF( "handler_sasp_receive(): packet size: %d\n", zepto_parsing_remaining_bytes( &po ) );
	}*/

	bool ipaad = SASP_IntraPacketAuthenticateAndDecrypt( key, mem_h, pid );
/*	if ( ipaad && for_sasp )
	{
		assert( debug_rem_b == 2 * SASP_ENC_BLOCK_SIZE );
	}*/

	PRINTF( "handler_sasp_receive(): PID: %02x %02x %02x %02x %02x %02x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
	if ( !ipaad )
	{
		INCREMENT_COUNTER( 12, "handler_sasp_receive(), garbage in" );
		PRINTF( "handler_sasp_receive(): BAD PACKET RECEIVED\n" );
		return SASP_RET_IGNORE;
	}
	bool for_sasp = SASP_is_for_sasp(  mem_h );

	// 2. Is it an Error Old Nonce Message
	if ( for_sasp )
	{
		zepto_response_to_request( mem_h ); // since now it's we who are the recipient of this packet
		INCREMENT_COUNTER( 13, "handler_sasp_receive(), intended for SASP" );
		uint8_t* nls = data+DATA_SASP_NONCE_LS_OFFSET;
		// now we need to extract a proposed value of nonce from the output
		// we do some strange manipulations: make output input, parse proper block, make input output
		// TODO: think whether it is possible to do in more sane way... somehow as it was before
		// uint8_t* new_nls = buffOut+2;
		parser_obj po1;
		zepto_parser_init( &po1, mem_h );
//		zepto_response_to_request( mem_h );
		zepto_parse_skip_block( &po1, 2 );
//		zepto_parse_read_block( &po1, stack + SASP_NONCE_SIZE, SASP_NONCE_SIZE );
		uint8_t new_nls[SASP_HEADER_SIZE];
		zepto_parser_decode_uint( &po1, new_nls, SASP_HEADER_SIZE );
/*		zepto_parser_init( &po1, mem_h );
		parser_obj po2;
		zepto_parser_init( &po2, mem_h );
		uint16_t packet_size = zepto_parsing_remaining_bytes( &po2 );
		zepto_parse_skip_block( &po2, packet_size );
		zepto_convert_part_of_request_to_response( mem_h, &po1, &po2 ); // finally, put where taken...*/

//		uint8_t* new_nls = stack + SASP_NONCE_SIZE;

		PRINTF( "handler_sasp_receive(): packet for SASP received...\n" );
		PRINTF( "   current  NLS: %02x %02x %02x %02x %02x %02x\n", nls[0], nls[1], nls[2], nls[3], nls[4], nls[5] );
		PRINTF( "   proposed NLS: %02x %02x %02x %02x %02x %02x\n", new_nls[0], new_nls[1], new_nls[2], new_nls[3], new_nls[4], new_nls[5] );
		// Recommended value of NLS starts from the second byte of the message; we should update NLS, if the recommended value is greater
		int8_t nonceCmp = SASP_NonceCompare( nls, new_nls ); // skipping First Byte and reserved byte
		if ( nonceCmp < 0 )
		{
			INCREMENT_COUNTER( 14, "handler_sasp_receive(), nonce las updated" );
//			SASP_NonceClearForSaspFlag( new_nls );
			memcpy( nls, new_nls, SASP_NONCE_SIZE );
			SASP_NonceLS_increment( nls );
			// TODO: shuold we do anything else?
			return SASP_RET_TO_HIGHER_LAST_SEND_FAILED;
		}
		PRINTF( "handler_sasp_receive(), for SASP, error old nonce, not applied\n" );
		return SASP_RET_IGNORE;
	}

	// 3. Compare nonces...
//	uint8_t ne[ SASP_HEADER_SIZE ];
//	uint8_t* ne_end = ne;
	uint8_t* nlw = data+DATA_SASP_NONCE_LW_OFFSET;
	int8_t nonceCmp = SASP_NonceCompare( pid, nlw );
	if ( nonceCmp <= 0 ) // error message must be prepared
	{
		INCREMENT_COUNTER( 15, "handler_sasp_receive(), error old nonce" );
		PRINTF( "handler_sasp_receive(): old nonce; packet for SASP is being prepared...\n" );
		PRINTF( "   nonce received: %02x %02x %02x %02x %02x %02x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		PRINTF( "   current    NLW: %02x %02x %02x %02x %02x %02x\n", nlw[0], nlw[1], nlw[2], nlw[3], nlw[4], nlw[5] );

		zepto_response_to_request( mem_h ); // we have to create a new response (with error old nonce)
		// we are to create a brand new output
		// TODO: think about faster and more straightforward approaches
/*		uint8_t* ins_ptr = stack;
		*(ins_ptr++) = 0;
		*(ins_ptr++) = 0; //  Byte
		memcpy( ins_ptr, nlw, SASP_NONCE_SIZE );
		ins_ptr += SASP_NONCE_SIZE;
		*sizeInOut = SASP_NONCE_SIZE+2;*/
		zepto_write_uint8( mem_h, 0x80 ); // First Byte; TODO: use bit field processing instead (should we?)
		zepto_write_uint8( mem_h, 0 ); // Reserved Byte
//		zepto_write_block( mem_h, nlw, SASP_NONCE_SIZE );
/*		zepto_parser_encode_uint( nlw, SASP_NONCE_SIZE, &ne_end );
		zepto_write_block( mem_h, ne, ne_end - ne );*/
		zepto_parser_encode_and_append_uint( mem_h, nlw, SASP_NONCE_SIZE );
		zepto_response_to_request( mem_h );

		uint8_t ne[ SASP_HEADER_SIZE ];
		memcpy( ne, pid, SASP_NONCE_SIZE );
//		SASP_NonceSetIntendedForSaspFlag( ne );
		SASP_EncryptAndAddAuthenticationData( mem_h/*, stack + SASP_NONCE_SIZE, stackSize - SASP_NONCE_SIZE*/,key, ne );
		PRINTF( "handler_sasp_receive(): ------------------- ERROR OLD NONCE WILL BE SENT ----------------------\n" );
		assert( ugly_hook_get_response_size( mem_h ) <= 39 );
		return SASP_RET_TO_LOWER_ERROR;
	}

	// 4. Finally, we have a brand new packet
//	memcpy( data+DATA_SASP_NONCE_LW_OFFSET, ne, SASP_NONCE_SIZE ); // update Nonce Low Watermark
	memcpy( data+DATA_SASP_NONCE_LW_OFFSET, pid, SASP_NONCE_SIZE ); // update Nonce Low Watermark
//	memcpy( data+DATA_SASP_LRPS_OFFSET, buffIn+SASP_HEADER_SIZE, SASP_TAG_SIZE ); // save packet signature
//	pid[ SASP_NONCE_SIZE - 1 ] &= (uint8_t)(0x7F);
	INCREMENT_COUNTER( 16, "handler_sasp_receive(), passed up" );
	return SASP_RET_TO_HIGHER_NEW;
}

uint8_t handler_sasp_get_packet_id( uint8_t* buffOut, int buffOutSize, uint8_t* data )
{
	SASP_NonceLS_increment( data + DATA_SASP_NONCE_LS_OFFSET );
	memcpy( buffOut, data + DATA_SASP_NONCE_LS_OFFSET, SASP_NONCE_SIZE );
//	buffOut[ SASP_NONCE_SIZE - 1 ] &= (uint8_t)(0x7F);
	return SASP_RET_NONCE;
}