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

#if !defined __SA_EAX_128_H__
#define __SA_EAX_128_H__

void eax_128_galois_times_2( uint8_t * buff )
{
	bool xor_required = *buff &0x80; // MSB
	for ( uint8_t i=0; i<15; i++ )
	{
		buff[i] <<= 1;
		buff[i] |= buff[i+1]>>7;
	}
	buff[15] <<= 1;
	if ( xor_required )
	{
		buff[15] ^= 0x87;
	}
}

void eax_128_generate_B( const uint8_t* key, uint8_t * buff )
{
	uint8_t zero[16];
	memset( zero, 0, 16 );
	sa_aes_128_encrypt_block( key, zero, buff );
	eax_128_galois_times_2( buff );
}

void eax_128_generate_P( const uint8_t* key, uint8_t * buff )
{
	uint8_t zero[16];
	memset( zero, 0, 16 );
	sa_aes_128_encrypt_block( key, zero, buff );
	eax_128_galois_times_2( buff );
	eax_128_galois_times_2( buff );
}

void eax_128_pad_block( const uint8_t* key, const uint8_t * block_in, uint8_t sz, uint8_t* block_out )
{
	assert( sz <= 16 );
	uint8_t p_b_buff[16];
	uint8_t i;
	if ( sz == 16 ) // full block
	{
		eax_128_generate_B( key, p_b_buff );
		for ( i=0; i<16; i++ )
			block_out[i] = block_in[i] ^ p_b_buff[i];
	}
	else
	{
		eax_128_generate_P( key, p_b_buff );
		for ( i=0; i<sz; i++ )
			block_out[i] = block_in[i] ^ p_b_buff[i];
		block_out[sz] = 0x80 ^ p_b_buff[sz]; // TODO: check value
		for ( i=sz+1; i<16; i++ )
			block_out[i] = p_b_buff[i];
	}
}

void eax_128_init_cbc( uint8_t* state )
{
	memset( state, 0, 16 );
}

void eax_128_update_cbc( const uint8_t* key, const uint8_t* block, uint8_t* cbc )
{
	uint8_t temp_blk[16];
	for ( uint8_t i=0; i<16; i++ )
		temp_blk[i] = cbc[i] ^ block[i];
	sa_aes_128_encrypt_block( key, temp_blk, cbc);
}

void eax_128_omac_t_of_single_block_message( const uint8_t* key, uint8_t t, const uint8_t* block_in, uint8_t sz, uint8_t* block_out )
{
	assert( sz <= 16 );
	uint8_t t_blk[16];
	memset( t_blk, 0, 16 );
	t_blk[15] = t;
	uint8_t state[16];
	if ( sz )
	{
		eax_128_init_cbc( state );
		eax_128_update_cbc( key, t_blk, state );
		// prepare last block
		eax_128_pad_block( key, block_in, sz, t_blk );
		eax_128_update_cbc( key, t_blk, state );
	}
	else
	{
		// just a single block
		eax_128_init_cbc( state );
		eax_128_pad_block( key, t_blk, 16, t_blk );
		eax_128_update_cbc( key, t_blk, state );
	}
	memcpy( block_out, state, 16 );
}

void eax_128_ctr_encrypt_and_step_ctr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t* block_out )
{
	uint8_t temp_blk[16];
	int8_t i;
	sa_aes_128_encrypt_block( key, ctr, temp_blk);
	for ( i=0; i<16; i++ )
		block_out[i] = temp_blk[i] ^ block_in[i];
	// increment counter
	for ( i=15; i>=0; i-- )
	{
		(ctr[i])++;
		if ( ctr[i] ) break; // else wrap around; proceed with more significant byte		
	}
}

void eax_128_init_ctr( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, uint8_t* ctr, uint8_t* tag_out )
{
	eax_128_omac_t_of_single_block_message( key, 0, nonce, nonce_sz, ctr );
	memcpy( tag_out, ctr, 16 );
}
		
void eax_128_init_cbc_for_nonzero_msg( const uint8_t* key, uint8_t* msg_cbc_val )
{
	memset( msg_cbc_val, 0, 16 ); // init cbc
	uint8_t t_blk[16];
	memset( t_blk, 0, 16 );
	t_blk[15] = 2;
	eax_128_update_cbc( key, t_blk, msg_cbc_val );
}

void eax_128_process_header( const uint8_t* key, const uint8_t* header, uint8_t header_sz, uint8_t* header_prim )
{
	eax_128_omac_t_of_single_block_message( key, 1, header, header_sz, header_prim );
}

void eax_128_process_nonterminating_block_encr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t* block_out, uint8_t* msg_cbc_val )
{
	eax_128_ctr_encrypt_and_step_ctr( key, ctr, block_in, block_out );
	eax_128_update_cbc( key, block_out, msg_cbc_val );
}

void eax_128_process_terminating_block_encr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t sz, uint8_t* block_out, uint8_t* msg_cbc_val )
{
	assert( sz != 0 );
	uint8_t t_blk[16];
	memcpy( t_blk, block_in, sz ); // just to ensure that a full-size block is formed
	eax_128_ctr_encrypt_and_step_ctr( key, ctr, t_blk, block_out );
	// pad for omac
	eax_128_pad_block( key, block_out, sz, t_blk );
	eax_128_update_cbc( key, t_blk, msg_cbc_val );
}

void eax_128_process_nonterminating_block_decr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t* block_out, uint8_t* msg_cbc_val )
{
	eax_128_update_cbc( key, block_in, msg_cbc_val );
	eax_128_ctr_encrypt_and_step_ctr( key, ctr, block_in, block_out );
}

void eax_128_process_terminating_block_decr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t sz, uint8_t* block_out, uint8_t* msg_cbc_val )
{
	assert( sz != 0 );
	// pad for omac
	uint8_t t_blk[16];
	eax_128_pad_block( key, block_in, sz, t_blk );
	eax_128_update_cbc( key, t_blk, msg_cbc_val );
	// decrypt
	memcpy( t_blk, block_in, sz ); // just to ensure that a full-size block is formed
	eax_128_ctr_encrypt_and_step_ctr( key, ctr, t_blk, block_out );
}

void eax_128_calc_tag( /*const uint8_t* key,*/ uint8_t* header_prim, uint8_t* msg_cbc_val, uint8_t* tag_out, uint8_t tag_out_sz )
{
/*	sa_aes_print_block_16("nonce_prim:     ", tag_out);
	sa_aes_print_block_16("header_prim:    ", header_prim);
	sa_aes_print_block_16("t_blk_out_omac: ", msg_cbc_val);*/
	for ( uint8_t i=0; i<tag_out_sz; i++ )
		tag_out[i] ^= header_prim[i] ^ msg_cbc_val[i];
}

void eax_128_SAMPLE_CODE_eax_of_single_block_message( const uint8_t* key, uint8_t* nonce, uint8_t nonce_sz, uint8_t* header, uint8_t header_sz, uint8_t* block_in, uint8_t sz, uint8_t* block_out, uint8_t* tag_out, uint8_t tag_out_sz )
{
	assert( sz <= 16 );
	assert( tag_out_sz <= 16 );
	uint8_t nonce_prim[16];
	uint8_t ctr[16];
	uint8_t header_prim[16];
	uint8_t t_blk[16];
	uint8_t t_blk_out[16];
	uint8_t t_blk_out_omac[16];
	// 1. convert nonce and init ctr
	eax_128_omac_t_of_single_block_message( key, 0, nonce, nonce_sz, nonce_prim ); // [[OK]]
	memcpy( ctr, nonce_prim, 16 );
	// 2. convert header
	eax_128_omac_t_of_single_block_message( key, 1, header, header_sz, header_prim ); // [?]
	// 3. encrypt message (single block in this case)
	memcpy( t_blk, block_in, sz ); // just to ensure that a full-size block is formed
	eax_128_ctr_encrypt_and_step_ctr( key, ctr, t_blk, t_blk_out ); // [[OK]]
	// 4. get omac of encrypted message (of single block in this case)
	eax_128_omac_t_of_single_block_message( key, 2, t_blk_out, sz, t_blk_out_omac ); // [?]
	// 5.0. print intermediate vals
	sa_aes_print_block_16("nonce_prim:     ", nonce_prim);
	sa_aes_print_block_16("header_prim:    ", header_prim);
	sa_aes_print_block_16("t_blk_out_omac: ", t_blk_out_omac);
	// 5. calculate and fill tag
	for ( uint8_t i=0; i<tag_out_sz; i++ )
		tag_out[i] = nonce_prim[i] ^ header_prim[i] ^ t_blk_out_omac[i];
	// 6. copy encrypted block
	memcpy( block_out, t_blk_out, 16 );
}

void eax_128_SAMPLE_CODE_eax_of_message( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, const uint8_t* header, uint8_t header_sz, uint8_t* msg, uint16_t msg_sz, uint8_t* msg_out, uint8_t* tag_out, uint8_t tag_out_sz )
{
	uint8_t ctr[16];
	uint8_t msg_cbc_val[16];
//	uint8_t header_prim[16];
	uint8_t* header_prim = ctr; // we will reuse it when ctr is no longer needed

	if ( msg_sz )
	{
		eax_128_init_ctr( key, nonce, nonce_sz, ctr, tag_out ); // ctr is for respective use; tag_out gets a copy of ctr to be used in finalizing
		eax_128_init_cbc_for_nonzero_msg( key, msg_cbc_val );

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
		eax_128_process_header( key, header, header_sz, header_prim );
		eax_128_calc_tag( header_prim, msg_cbc_val, tag_out, tag_out_sz );
	}
	else // zero length is a special case
	{

		eax_128_init_ctr( key, nonce, nonce_sz, msg_cbc_val, tag_out ); // get cbc of nonce

		memset( ctr, 0, 16 );
		ctr[15] = 2;

		eax_128_init_cbc( msg_cbc_val );
		eax_128_pad_block( key, ctr, 16, ctr );
		eax_128_update_cbc( key, ctr, msg_cbc_val );

		// finalize
		eax_128_process_header( key, header, header_sz, header_prim );
		eax_128_calc_tag( header_prim, msg_cbc_val, tag_out, tag_out_sz );
	}
}

bool eax_128_SAMPLE_CODE_check_eax_of_message( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, const uint8_t* header, uint8_t header_sz, uint8_t* msg, uint16_t msg_sz, uint8_t* msg_out, uint8_t* tag_received, uint8_t tag_sz )
{
	uint8_t tag_out[16];
	uint8_t ctr[16];
	uint8_t msg_cbc_val[16];
//	uint8_t header_prim[16];
	uint8_t* header_prim = ctr; // we will reuse it when ctr is no longer needed

	if ( msg_sz )
	{
		eax_128_init_ctr( key, nonce, nonce_sz, ctr, tag_out ); // ctr is for respective use; tag_out gets a copy of ctr to be used in finalizing
		eax_128_init_cbc_for_nonzero_msg( key, msg_cbc_val );

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
		eax_128_process_header( key, header, header_sz, header_prim );
		eax_128_calc_tag( header_prim, msg_cbc_val, tag_out, tag_sz );

		for ( uint8_t i=0; i<tag_sz; i++ )
			if ( tag_out[i] != tag_received[i] ) return false;
		return true;
	}
	else // zero length is a special case
	{

		eax_128_init_ctr( key, nonce, nonce_sz, msg_cbc_val, tag_out ); // get cbc of nonce

		memset( ctr, 0, 16 );
		ctr[15] = 2;

		eax_128_init_cbc( msg_cbc_val );
		eax_128_pad_block( key, ctr, 16, ctr );
		eax_128_update_cbc( key, ctr, msg_cbc_val );

		// finalize
		eax_128_process_header( key, header, header_sz, header_prim );
		eax_128_calc_tag( /*const uint8_t* key,*/ header_prim, msg_cbc_val, tag_out, tag_sz );

		for ( uint8_t i=0; i<tag_sz; i++ )
			if ( tag_out[i] != tag_received[i] ) return false;
		return true;
	}
}

#endif // __SA_EAX_128_H__