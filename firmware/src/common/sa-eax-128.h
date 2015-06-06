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

//void eax_128_init_ctr( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, uint8_t* ctr, uint8_t* tag_out );
//void eax_128_process_header( const uint8_t* key, const uint8_t* header, uint8_t header_sz, uint8_t* header_prim );
void eax_128_process_nonterminating_block_encr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t* block_out, uint8_t* msg_cbc_val );
void eax_128_process_terminating_block_encr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t sz, uint8_t* block_out, uint8_t* msg_cbc_val );
void eax_128_process_nonterminating_block_decr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t* block_out, uint8_t* msg_cbc_val );
void eax_128_process_terminating_block_decr( const uint8_t* key, uint8_t* ctr, const uint8_t* block_in, uint8_t sz, uint8_t* block_out, uint8_t* msg_cbc_val );
//void eax_128_calc_tag( /*const uint8_t* key,*/ uint8_t* header_prim, uint8_t* msg_cbc_val, uint8_t* tag_out, uint8_t tag_out_sz );


void eax_128_init_for_nonzero_msg( const uint8_t* key, const uint8_t* nonce, uint8_t nonce_sz, uint8_t* ctr, uint8_t* msg_cbc_val, uint8_t* tag );
void eax_128_finalize_for_nonzero_msg( const uint8_t* key, const uint8_t* header, uint8_t header_sz, uint8_t* ctr, uint8_t* msg_cbc_val, uint8_t* tag, uint8_t tag_out_sz );
void eax_128_calc_tag_of_zero_msg( const uint8_t* key, const uint8_t* header, uint8_t header_sz, const uint8_t* nonce, uint8_t nonce_sz, uint8_t* tag, uint8_t tag_out_sz );



#endif // __SA_EAX_128_H__