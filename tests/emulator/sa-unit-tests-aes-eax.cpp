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

#include "sa-common.h"
#include "sa-aes-128.h"
#include "sa-eax-128.h"
#include "sa-unit-tests-aes-eax.h"

void sa_aes_print_msg(const char* name, const uint8_t* msg, uint16_t msg_sz)
{
    printf("%s", name);
    for (uint16_t i = 0; i < msg_sz; (i)++)
    	printf("%02x ", msg[i]);
    printf("\n");
}


void sa_aes_print_block_16(const char* name, uint8_t* block)
{
    printf("%s", name);
    for (uint8_t i = 0; i < 16; (i)++)
    	printf("%02x ", block[i]);
    printf("\n");
}

void eax_128_SAMPLE_CODE_eax_of_message( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, const uint8_t* header, uint8_t header_sz, const uint8_t* msg, uint16_t msg_sz, uint8_t* msg_out, uint8_t* tag, uint8_t tag_sz )
{
	if ( msg_sz )
	{
		uint8_t ctr[16];
		uint8_t msg_cbc_val[16];
//		uint8_t header_prim[16];
//		uint8_t* header_prim = ctr; // we will reuse it when ctr is no longer needed
		eax_128_init_for_nonzero_msg( key, nonce, nonce_sz, ctr, msg_cbc_val, tag );

		// process blocks
		// in actual implementation using the same logic it's a place to write resulting packet encrypted part
		while ( msg_sz >= 16 )
		{
			eax_128_process_nonterminating_block_encr( key, ctr, msg, msg_out, msg_cbc_val );
			msg += 16;
			msg_out += 16;
			msg_sz -= 16;
		}
		eax_128_process_terminating_block_encr( key, ctr, msg, msg_sz, msg_out, msg_cbc_val );

		// finalize
		eax_128_finalize_for_nonzero_msg( key, header, header_sz, ctr, msg_cbc_val, tag, tag_sz );
	}
	else // zero length is a special case
	{
		eax_128_calc_tag_of_zero_msg( key, header, header_sz, nonce, nonce_sz, tag, tag_sz );
	}
}

bool eax_128_SAMPLE_CODE_check_eax_of_message( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, const uint8_t* header, uint8_t header_sz, uint8_t* msg, uint16_t msg_sz, uint8_t* msg_out, uint8_t* tag_received, uint8_t tag_sz )
{
	uint8_t tag_calculted[16];
	uint8_t ctr[16];
	uint8_t msg_cbc_val[16];
//	uint8_t header_prim[16];
	uint8_t* header_prim = ctr; // we will reuse it when ctr is no longer needed

	if ( msg_sz )
	{
		eax_128_init_for_nonzero_msg( key, nonce, nonce_sz, ctr, msg_cbc_val, tag_calculted );

		// process blocks
		// in actual implementation using the same logic it's a place to write resulting packet encrypted part
		while ( msg_sz >= 16 )
		{
			eax_128_process_nonterminating_block_decr( key, ctr, msg, msg_out, msg_cbc_val );
			msg += 16;
			msg_out += 16;
			msg_sz -= 16;
		}
		eax_128_process_terminating_block_decr( key, ctr, msg, msg_sz, msg_out, msg_cbc_val );

		// finalize
		eax_128_finalize_for_nonzero_msg( key, header, header_sz, ctr, msg_cbc_val, tag_calculted, tag_sz );

		for ( uint8_t i=0; i<tag_sz; i++ )
			if ( tag_calculted[i] != tag_received[i] ) return false;
		return true;
	}
	else // zero length is a special case
	{
		eax_128_calc_tag_of_zero_msg( key, header, header_sz, nonce, nonce_sz, tag_calculted, tag_sz );

		for ( uint8_t i=0; i<tag_sz; i++ )
			if ( tag_calculted[i] != tag_received[i] ) return false;
		return true;
	}
}

uint8_t sa_aes_hex_char_to_uint8( char ch )
{
	if ( ch >= '0' && ch <= '9' )
		return ch - '0';
	else if ( ch >= 'a' && ch <= 'f' )
		return ch - 'a' + 10;
	else if ( ch >= 'A' && ch <= 'F' )
		return ch - 'A' + 10;
	assert(0);
	return 0xff;
}

uint8_t sa_aes_string_to_block( const char* str, uint8_t* block ) // returns size
{
    for (uint8_t i = 0; i < 256; (i)++)
	{
		if ( str[i*2] == 0 ) return i;
		assert( str[i*2+1] );
		block[i] = ( sa_aes_hex_char_to_uint8( str[i*2] ) << 4 ) + sa_aes_hex_char_to_uint8( str[i*2+1] );
	}
	return 0;
}

int test_aes()
{
	uint8_t res[256];
	unsigned char key[16];
	unsigned char message[256];
	uint8_t true_enc_msg[256];
	memset( message, 0, 256 );
	memset( key, 0, 16 );

/*
	uint8_t msg_sz = sa_aes_string_to_block( "80000000000000000000000000000000", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "3ad78e726c1ec02b7ebfe92b23d9ec34", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "c0000000000000000000000000000000", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "aae5939c8efdf2f04e60b9fe7117b2c2", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "fffffffffffffff00000000000000000", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "6898d4f42fa7ba6a10ac05e87b9f2080", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "ffffffffffffffffffffffffffffff80", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "d1788f572d98b2b16ec5d5f3922b99bc", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "fffffffffffffffffffffffffffffff0", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "f9b0fda0c4a898f5b9e6f661c4ce4d07", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "fffffffffffffffffffffffffffffff8", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "8ade895913685c67c5269f8aae42983e", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "fffffffffffffffffffffffffffffffc", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "39bde67d5c8ed8a8b1c37eb8fa9f5ac0", true_enc_msg );

	uint8_t msg_sz = sa_aes_string_to_block( "fffffffffffffffffffffffffffffffe", message );
	uint8_t true_c_sz = sa_aes_string_to_block( "5c005e72c1418c44f569f2ea33ba54f3", true_enc_msg );
*/

	uint8_t msg_sz = sa_aes_string_to_block( "ffffffffffffffffffffffffffffffff", key );
	uint8_t true_c_sz = sa_aes_string_to_block( "a1f6258c877d5fcd8964484538bfc92c", true_enc_msg );

	sa_aes_128_encrypt_block(key, message, res);
	sa_aes_print_block_16("encrypted: ", res);
	sa_aes_print_block_16("true encr: ", true_enc_msg);
	return 0;
}

#include "sa-eax-128.h"
#define SA_UNIT_TEST_EAX_MSG_MAX_SIZE 32
bool test_eax(  const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, const uint8_t* header, uint8_t header_sz, const uint8_t* msg, const uint8_t* true_enc_msg, uint16_t msg_sz, const uint8_t* true_tag, uint8_t tag_sz  )
{
	uint8_t tag[16];
	uint8_t enc_msg[SA_UNIT_TEST_EAX_MSG_MAX_SIZE];
	uint8_t dec_msg[SA_UNIT_TEST_EAX_MSG_MAX_SIZE];
	uint16_t i;
	bool ok = true;

	eax_128_SAMPLE_CODE_eax_of_message( key, nonce, nonce_sz, header, header_sz, msg, msg_sz, enc_msg, tag, 16 );

	for ( i=0;i<tag_sz; i++ )
		ok = ok && tag[i] == true_tag[i];
	if ( !ok )
	{
		sa_aes_print_msg("tag:   ", tag, tag_sz );
		sa_aes_print_msg("tr.tag:", true_tag, tag_sz );
		assert( NULL == "EAX: tag calculation failed\n" );
		return false;
	}
	for ( i=0;i<msg_sz; i++ )
		ok = ok && enc_msg[i] == true_enc_msg[i];
	if ( !ok )
	{
		sa_aes_print_msg("crypto:", enc_msg, msg_sz);
		sa_aes_print_msg("tr.cr.:", true_enc_msg, msg_sz);
		assert( NULL == "EAX: encryption failed\n" );
		return false;
	}

	ok = eax_128_SAMPLE_CODE_check_eax_of_message( key, nonce, nonce_sz, header, header_sz, enc_msg, msg_sz, dec_msg, tag, 16 );

	if ( !ok )
	{
		assert( NULL == "EAX: authentication failed\n" );
		return false;
	}
	for ( i=0;i<msg_sz; i++ )
		ok = ok && dec_msg[i] == msg[i];

	if ( !ok )
	{
		sa_aes_print_msg("msg:     ", msg, msg_sz);
		sa_aes_print_msg("decr. msg:", dec_msg, msg_sz);
		assert( NULL == "EAX: decryption failed\n" );
		return false;
	}

	return true;
}

int test_eax()
{
	uint8_t key[16];
	uint8_t msg[160];
	uint8_t nonce[16];
	uint8_t header[16];
	uint8_t true_tag[16];
	uint8_t true_enc_msg[SA_UNIT_TEST_EAX_MSG_MAX_SIZE];

	uint8_t key_sz;
	uint8_t msg_sz;
	uint8_t nonce_sz;
	uint8_t header_sz;
	uint8_t true_t_sz;
	uint8_t true_c_sz;

	bool ok = true;

	key_sz = sa_aes_string_to_block( "233952DEE4D5ED5F9B9C6D6FF80FF478", key );
	assert( key_sz == 16 );
	msg_sz = sa_aes_string_to_block( "", msg );
	nonce_sz = sa_aes_string_to_block( "62EC67F9C3A4A407FCB2A8C49031A8B3", nonce );
	header_sz = sa_aes_string_to_block( "6BFB914FD07EAE6B", header );
	true_t_sz = sa_aes_string_to_block( "E037830E8389F27B025A2D6527E79D01", true_tag );
	true_c_sz = sa_aes_string_to_block( "", true_enc_msg );
	ok = ok && test_eax( key, nonce, nonce_sz, header, header_sz, msg, true_enc_msg, msg_sz, true_tag, true_t_sz );

	key_sz = sa_aes_string_to_block( "8395FCF1E95BEBD697BD010BC766AAC3", key );
	assert( key_sz == 16 );
	msg_sz = sa_aes_string_to_block( "CA40D7446E545FFAED3BD12A740A659FFBBB3CEAB7", msg );
	nonce_sz = sa_aes_string_to_block( "22E7ADD93CFC6393C57EC0B3C17D6B44", nonce );
	header_sz = sa_aes_string_to_block( "126735FCC320D25A", header );
	true_t_sz = sa_aes_string_to_block( "CFC46AFC253B4652B1AF3795B124AB6E", true_tag );
	true_c_sz = sa_aes_string_to_block( "CB8920F87A6C75CFF39627B56E3ED197C552D295A7", true_enc_msg );
	assert( msg_sz == true_c_sz );
	ok = ok && test_eax( key, nonce, nonce_sz, header, header_sz, msg, true_enc_msg, msg_sz, true_tag, true_t_sz );

	key_sz = sa_aes_string_to_block( "D07CF6CBB7F313BDDE66B727AFD3C5E8", key );
	assert( key_sz == 16 );
	msg_sz = sa_aes_string_to_block( "481C9E39B1", msg );
	nonce_sz = sa_aes_string_to_block( "8408DFFF3C1A2B1292DC199E46B7D617", nonce );
	header_sz = sa_aes_string_to_block( "33CCE2EABFF5A79D", header );
	true_t_sz = sa_aes_string_to_block( "D4C168A4225D8E1FF755939974A7BEDE", true_tag );
	true_c_sz = sa_aes_string_to_block( "632A9D131A", true_enc_msg );
	assert( msg_sz == true_c_sz );
	ok = ok && test_eax( key, nonce, nonce_sz, header, header_sz, msg, true_enc_msg, msg_sz, true_tag, true_t_sz );

	key_sz = sa_aes_string_to_block( "BD8E6E11475E60B268784C38C62FEB22", key );
	assert( key_sz == 16 );
	msg_sz = sa_aes_string_to_block( "4DE3B35C3FC039245BD1FB7D", msg );
	nonce_sz = sa_aes_string_to_block( "6EAC5C93072D8E8513F750935E46DA1B", nonce );
	header_sz = sa_aes_string_to_block( "D4482D1CA78DCE0F", header );
	true_t_sz = sa_aes_string_to_block( "ABB8644FD6CCB86947C5E10590210A4F", true_tag );
	true_c_sz = sa_aes_string_to_block( "835BB4F15D743E350E728414", true_enc_msg );
	ok = ok && test_eax( key, nonce, nonce_sz, header, header_sz, msg, true_enc_msg, msg_sz, true_tag, true_t_sz );
	printf( ok ? "EAX test OK\n" :  "EAX test FAILED\n" );
	return 0;
}

void test_aes_and_eax_128()
{
//	test_aes();
	test_eax(); // implicitly tests both eas and eax
}