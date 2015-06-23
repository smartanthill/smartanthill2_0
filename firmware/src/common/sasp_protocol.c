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
#include "zepto_mem_mngmt.h"
#include "sa_eax_128.h"

static 	SASP_DATA sasp_data;



void sasp_init_at_lifestart( /*SASP_DATA* sasp_data*/ )
{
	sa_uint48_set_zero( sasp_data.nonce_lw );
	sa_uint48_set_zero( sasp_data.nonce_ls );
	sa_uint48_increment( sasp_data.nonce_ls );

	eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LW_ID, sasp_data.nonce_lw );
	eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LS_ID, sasp_data.nonce_ls );
}

void sasp_restore_from_backup( /*SASP_DATA* sasp_data*/ )
{
	uint8_t size;

	eeprom_read( EEPROM_SLOT_DATA_SASP_NONCE_LW_ID, sasp_data.nonce_lw );
	eeprom_read( EEPROM_SLOT_DATA_SASP_NONCE_LS_ID, sasp_data.nonce_ls );
}

void SASP_increment_nonce_last_sent( /*SASP_DATA* sasp_data*/ )
{
	sa_uint48_increment( sasp_data.nonce_ls );
	// TODO: make sure this is a reliable approach
	if ( sa_uint48_get_byte( sasp_data.nonce_ls, 0 ) == 1 )
	{
		sasp_nonce_type future;
		memcpy( future, sasp_data.nonce_ls, SASP_NONCE_TYPE_SIZE );
		sa_uint48_roundup_to_the_nearest_multiple_of_0x100( future );
		eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LS_ID, future );
	}
}

void SASP_update_nonce_low_watermark( /*SASP_DATA* sasp_data, */const sasp_nonce_type new_val )
{
	sasp_nonce_type future;
	sa_uint48_init_by( future, sasp_data.nonce_lw );
	sa_uint48_init_by( sasp_data.nonce_lw, new_val ); // update Nonce Low Watermark
	sa_uint48_roundup_to_the_nearest_multiple_of_0x100( future ); // now 'future' has a value supposedly saved in eeprom
	if ( sa_uint48_compare( new_val, future ) >= 0 )
	{
		sa_uint48_init_by( future, sasp_data.nonce_lw );
		sa_uint48_roundup_to_the_nearest_multiple_of_0x100( future ); // now 'future' has a value supposedly saved in eeprom
		eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LW_ID, future );
	}
}

INLINE
void SASP_make_nonce_for_encryption( const sasp_nonce_type packet_id, uint8_t master_slave_bit, uint8_t nonce[16] )
{
	memset( nonce, 0, 16 );
	int8_t i;
	for ( i=0; i<SASP_NONCE_TYPE_SIZE; i++ )
	{
		nonce[i] = sa_uint48_get_byte( packet_id, i );
	}
	nonce[15] = master_slave_bit;
}

INLINE
void SASP_EncryptAndAddAuthenticationData( REQUEST_REPLY_HANDLE mem_h, const uint8_t* key, const sa_uint48_t packet_id )
{
	// msgSize covers the size of message_byte_sequence
	// (byte with MASTER_SLAVE_BIT) | nonce concatenation is used as a full nonce
	// the output is formed as follows: prefix (with nonce; not a EAX header) | encrypted data | tag

	// data under encryption is as follows: first_byte | message_byte_sequence

	bool read_ok;

	uint8_t nonce[16];
	SASP_make_nonce_for_encryption( packet_id, MASTER_SLAVE_BIT, nonce );

	// intermediate buffers for EAX
	uint8_t tag[SASP_TAG_SIZE];

	// init parser object
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );


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
			ZEPTO_DEBUG_ASSERT( read_ok );
			eax_128_process_nonterminating_block_encr( key, ctr, block, block, msg_cbc_val );
			zepto_write_block( mem_h, block, SASP_ENC_BLOCK_SIZE );
		}
		uint16_t remaining_sz = zepto_parsing_remaining_bytes( &po );
		read_ok = zepto_parse_read_block( &po, block, remaining_sz );
		ZEPTO_DEBUG_ASSERT( read_ok );
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
	zepto_parser_encode_and_prepend_uint( mem_h, packet_id, SASP_NONCE_SIZE );
}

bool SASP_IntraPacketAuthenticateAndDecrypt( const uint8_t* key, REQUEST_REPLY_HANDLE mem_h, sa_uint48_t pid )
{
	// input data structure: prefix (with nonce; not a part of EAX header) | encrypted data | tag
	// Structure of output buffer: first_byte | byte_sequence
	// pid (Packet ID) is formed from prefix data

	uint8_t block[ SASP_ENC_BLOCK_SIZE ];
	uint8_t tag_calculted[16];
	uint8_t ctr[16];
	uint8_t msg_cbc_val[16];

	// init parser object
	parser_obj po;
	zepto_parser_init( &po, mem_h );

	// read and form PID
	sa_uint48_t nonce_received;
	zepto_parser_decode_encoded_uint_as_sa_uint48( &po, nonce_received );
	sa_uint48_init_by( pid, nonce_received );
	uint8_t nonce[ 16 ];
	SASP_make_nonce_for_encryption( nonce_received, 1 - MASTER_SLAVE_BIT, nonce );

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
	uint8_t i;
	for ( i=0; i<SASP_TAG_SIZE; i++ )
		if ( tag_calculted[i] != block[i] ) return false;

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
	zepto_parser_init_by_parser( &po1, &po );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );

	return for_sasp;
}

#ifdef _DEBUG
void DEBUG_SASP_EncryptAndAddAuthenticationDataChecked( MEMORY_HANDLE mem_h, const uint8_t* key, const sa_uint48_t nonce )
{
	uint8_t header[6];

	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );
	uint16_t inisz = zepto_parsing_remaining_bytes( &po );

	ZEPTO_DEBUG_PRINTF_7( "handler_sasp_send(): nonce used: %02x %02x %02x %02x %02x %02x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );
	ZEPTO_DEBUG_PRINTF_2( "handler_sasp_send(): size: %d\n", inisz );

	uint16_t encr_sz, decr_sz;
	uint8_t inimsg[1024]; zepto_parse_read_block( &po, inimsg, inisz );
	uint8_t checkedMsg[1024];
	uint8_t dbg_nonce[6];

	// do required job
	SASP_EncryptAndAddAuthenticationData( mem_h, key, nonce );

	// CHECK RESULTS

	// init parser object
	zepto_parser_init( &po, mem_h );
	zepto_parser_init( &po1, mem_h );


	// TODO: if this debug code remains in use, lines below must be replaced by getting a new handle
	uint8_t x_buff1_x2[1024];

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
	ZEPTO_DEBUG_ASSERT( ipaad );
	zepto_response_to_request( MEMORY_HANDLE_DBG_TMP );
	zepto_parser_init( &po, MEMORY_HANDLE_DBG_TMP );
	decr_sz = zepto_parsing_remaining_bytes( &po );
	zepto_parse_read_block( &po, checkedMsg, decr_sz );

	ZEPTO_DEBUG_PRINTF_7( "handler_sasp_send():     nonce: %02x %02x %02x %02x %02x %02x\n", nonce[0], nonce[1], nonce[2], nonce[3], nonce[4], nonce[5] );
	ZEPTO_DEBUG_PRINTF_7( "handler_sasp_send(): dbg_nonce: %02x %02x %02x %02x %02x %02x\n", dbg_nonce[0], dbg_nonce[1], dbg_nonce[2], dbg_nonce[3], dbg_nonce[4], dbg_nonce[5] );
	ZEPTO_DEBUG_ASSERT( ipaad );
	ZEPTO_DEBUG_ASSERT( decr_sz == inisz );
	ZEPTO_DEBUG_ASSERT( memcmp( nonce, dbg_nonce, 6 ) == 0 );
	uint8_t k;
	for ( k=0; k<decr_sz; k++ )
		ZEPTO_DEBUG_ASSERT( inimsg[k] == checkedMsg[k] );
}
#endif

uint8_t handler_sasp_send( const uint8_t* key, const sa_uint48_t packet_id, MEMORY_HANDLE mem_h/*, SASP_DATA* sasp_data*/ )
{
	ZEPTO_DEBUG_ASSERT( sa_uint48_compare( packet_id, sasp_data.nonce_ls ) >= 0 );

#ifdef _DEBUG
//	DEBUG_SASP_EncryptAndAddAuthenticationDataChecked( mem_h, key, packet_id );
	SASP_EncryptAndAddAuthenticationData( mem_h, key, packet_id );
#else
	SASP_EncryptAndAddAuthenticationData( mem_h, key, packet_id );
#endif

	INCREMENT_COUNTER( 10, "handler_sasp_send()" );
	return SASP_RET_TO_LOWER_REGULAR;
}

uint8_t handler_sasp_receive( const uint8_t* key, uint8_t* pid, MEMORY_HANDLE mem_h/*, SASP_DATA* sasp_data*/ )
{
	INCREMENT_COUNTER( 11, "handler_sasp_receive()" );
	// 1. Perform intra-packet authentication
	parser_obj po;
	zepto_parser_init( &po, mem_h );
	uint16_t packet_size = zepto_parsing_remaining_bytes( &po );
	if ( packet_size == 0 )
	{
		return SASP_RET_IGNORE;
	}

	bool ipaad = SASP_IntraPacketAuthenticateAndDecrypt( key, mem_h, pid );

	ZEPTO_DEBUG_PRINTF_7( "handler_sasp_receive(): PID: %02x %02x %02x %02x %02x %02x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
	if ( !ipaad )
	{
		INCREMENT_COUNTER( 12, "handler_sasp_receive(), garbage in" );
		ZEPTO_DEBUG_PRINTF_1( "handler_sasp_receive(): BAD PACKET RECEIVED\n" );
		return SASP_RET_IGNORE;
	}
	bool for_sasp = SASP_is_for_sasp(  mem_h );

	// 2. Is it an Error Old Nonce Message
	if ( for_sasp )
	{
		zepto_response_to_request( mem_h ); // since now it's we who are the recipient of this packet
		INCREMENT_COUNTER( 13, "handler_sasp_receive(), intended for SASP" );

		parser_obj po1;
		zepto_parser_init( &po1, mem_h );
		zepto_parse_skip_block( &po1, 2 );
		sa_uint48_t new_nls;
		zepto_parser_decode_encoded_uint_as_sa_uint48( &po1, new_nls );

#ifdef SA_DEBUG
		ZEPTO_DEBUG_PRINTF_1( "handler_sasp_receive(): packet for SASP received...\n" );
		uint8_t* nls = sasp_data.nonce_ls;
		ZEPTO_DEBUG_PRINTF_7( "   current  NLS: %02x %02x %02x %02x %02x %02x\n", nls[0], nls[1], nls[2], nls[3], nls[4], nls[5] );
		ZEPTO_DEBUG_PRINTF_7( "   proposed NLS: %02x %02x %02x %02x %02x %02x\n", new_nls[0], new_nls[1], new_nls[2], new_nls[3], new_nls[4], new_nls[5] );
#endif

		// Recommended value of NLS starts from the second byte of the message; we should update NLS, if the recommended value is greater
		int8_t nonceCmp = sa_uint48_compare( sasp_data.nonce_ls, new_nls ); // skipping First Byte and reserved byte
		if ( nonceCmp < 0 )
		{
			INCREMENT_COUNTER( 14, "handler_sasp_receive(), nonce las updated" );
			memcpy( nls, new_nls, SASP_NONCE_SIZE );
//			sa_uint48_increment( sasp_data.nonce_ls );
			SASP_increment_nonce_last_sent( sasp_data );
			// TODO: shuold we do anything else?
			return SASP_RET_TO_HIGHER_LAST_SEND_FAILED;
		}
		ZEPTO_DEBUG_PRINTF_1( "handler_sasp_receive(), for SASP, error old nonce, not applied\n" );
		return SASP_RET_IGNORE;
	}

	// 3. Compare nonces...
	int8_t nonceCmp = sa_uint48_compare( pid, sasp_data.nonce_lw );
	if ( nonceCmp <= 0 ) // error message must be prepared
	{
#ifdef SA_DEBUG
		INCREMENT_COUNTER( 15, "handler_sasp_receive(), error old nonce" );
		ZEPTO_DEBUG_PRINTF_1( "handler_sasp_receive(): old nonce; packet for SASP is being prepared...\n" );
		ZEPTO_DEBUG_PRINTF_7( "   nonce received: %02x %02x %02x %02x %02x %02x\n", pid[0], pid[1], pid[2], pid[3], pid[4], pid[5] );
		uint8_t* nlw = sasp_data.nonce_lw;
		ZEPTO_DEBUG_PRINTF_7( "   current    NLW: %02x %02x %02x %02x %02x %02x\n", nlw[0], nlw[1], nlw[2], nlw[3], nlw[4], nlw[5] );
#endif

		zepto_response_to_request( mem_h ); // we have to create a new response (with error old nonce)
		// we are to create a brand new output
		zepto_write_uint8( mem_h, 0x80 ); // First Byte; TODO: use bit field processing instead (should we?)
		zepto_write_uint8( mem_h, 0 ); // Reserved Byte
		zepto_parser_encode_and_append_uint( mem_h, sasp_data.nonce_lw, SASP_NONCE_SIZE );
		zepto_response_to_request( mem_h );

		uint8_t ne[ SASP_HEADER_SIZE ];
		memcpy( ne, pid, SASP_NONCE_SIZE );
		SASP_EncryptAndAddAuthenticationData( mem_h,key, ne );
		ZEPTO_DEBUG_PRINTF_1( "handler_sasp_receive(): ------------------- ERROR OLD NONCE WILL BE SENT ----------------------\n" );
//		ZEPTO_DEBUG_ASSERT( ugly_hook_get_response_size( mem_h ) <= 39 );
		return SASP_RET_TO_LOWER_ERROR;
	}

	// 4. Finally, we have a brand new packet
//	sa_uint48_init_by( sasp_data.nonce_lw, pid ); // update Nonce Low Watermark
	SASP_update_nonce_low_watermark( /*sasp_data, */pid );
	INCREMENT_COUNTER( 16, "handler_sasp_receive(), passed up" );
	return SASP_RET_TO_HIGHER_NEW;
}

uint8_t handler_sasp_get_packet_id( sa_uint48_t buffOut, int buffOutSize/*, SASP_DATA* sasp_data*/ )
{
	SASP_increment_nonce_last_sent( sasp_data );
	sa_uint48_init_by( buffOut, sasp_data.nonce_ls );
	return SASP_RET_NONCE;
}

uint8_t handler_sasp_save_state( /*SASP_DATA* sasp_data*/ )
{
	eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LW_ID, sasp_data.nonce_lw );
	eeprom_write( EEPROM_SLOT_DATA_SASP_NONCE_LS_ID, sasp_data.nonce_ls );
}