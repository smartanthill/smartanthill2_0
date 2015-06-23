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

#include "../../firmware/src/common/sa_common.h"

#define SHA256_BYTES_TO_UINT32( num, bytes ) {num = (bytes)[0]; num <<= 8; num += (bytes)[1]; num <<= 8; num += (bytes)[2]; num <<= 8; num += (bytes)[3];}
#define SHA256_UINT32_TO_BYTES( num, bytes ) {(bytes)[3] = (uint8_t)(num); (bytes)[2] = (uint8_t)((num)>>8); (bytes)[1] = (uint8_t)((num)>>16); (bytes)[0] = (uint8_t)((num)>>24);}
#define SHA256_ROTATE_RIGHT( value, shift ) ((value >> shift) | (value << (32 - shift)))


const unsigned int sha256_round_constants[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

typedef struct _sha256_round_data
{
	unsigned int h0;
	unsigned int h1;
	unsigned int h2;
	unsigned int h3;
	unsigned int h4;
	unsigned int h5;
	unsigned int h6;
	unsigned int h7;

	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;
	unsigned int e;
	unsigned int f;
	unsigned int g;
	unsigned int h;

	uint8_t round;
} sha256_round_data;

void sha256_round(sha256_round_data* data, unsigned int w)
{
	if (data->round == 0)
	{
		data->a = data->h0;
		data->b = data->h1;
		data->c = data->h2;
		data->d = data->h3;
		data->e = data->h4;
		data->f = data->h5;
		data->g = data->h6;
		data->h = data->h7;
	}

	unsigned int temp1, temp2, S1, S0, ch, maj;

	S1 = SHA256_ROTATE_RIGHT(data->e, 6) ^ SHA256_ROTATE_RIGHT(data->e, 11) ^ SHA256_ROTATE_RIGHT(data->e, 25);
	ch = (data->e & data->f) ^ ((~data->e) & data->g);
	temp1 = data->h + S1 + ch + sha256_round_constants[data->round] + w;
	S0 = SHA256_ROTATE_RIGHT(data->a, 2) ^ SHA256_ROTATE_RIGHT(data->a, 13) ^ SHA256_ROTATE_RIGHT(data->a, 22);
	maj = (data->a & data->b) ^ (data->a & data->c) ^ (data->b & data->c);
	temp2 = S0 + maj;

	data->h = data->g;
	data->g = data->f;
	data->f = data->e;
	data->e = data->d + temp1;
	data->d = data->c;
	data->c = data->b;
	data->b = data->a;
	data->a = temp1 + temp2;

	if (data->round == 63)
	{
		data->h0 += data->a;
		data->h1 += data->b;
		data->h2 += data->c;
		data->h3 += data->d;
		data->h4 += data->e;
		data->h5 += data->f;
		data->h6 += data->g;
		data->h7 += data->h;
		data->round = 0;
	}
	else
		(data->round)++;
}

void sha256(const uint8_t* msg, uint16_t len, uint8_t* hash)
{
	sha256_round_data rdata;
	uint16_t total_chunks;
	uint16_t processed_chunks;
	unsigned int words[16];
	uint8_t i;
	unsigned int s0, s1;

	total_chunks = (len + (64 - (len & 0x3f))) >> 6;

	// 1. initialize
	rdata.h0 = 0x6a09e667;
	rdata.h1 = 0xbb67ae85;
	rdata.h2 = 0x3c6ef372;
	rdata.h3 = 0xa54ff53a;
	rdata.h4 = 0x510e527f;
	rdata.h5 = 0x9b05688c;
	rdata.h6 = 0x1f83d9ab;
	rdata.h7 = 0x5be0cd19;
	rdata.round = 0;

	// 2. process complete chunks

	for (processed_chunks = 0; processed_chunks < total_chunks - 1; processed_chunks++)
	{
		const uint8_t* chunk = msg + processed_chunks * 64;
		memset(words, 0, 16 * sizeof(unsigned int));
		for (i = 0; i<16; i++)
		{
			SHA256_BYTES_TO_UINT32( words[i], chunk+(i<<2) )
			sha256_round( &rdata, words[i] );
		}

		for (i = 16; i < 64; i++)
		{
			s0 = SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 7) ^ SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 18) ^ (words[(i - 15) & 0xf] >> 3);
			s1 = SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 17) ^ SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 19) ^ (words[(i - 2) & 0xf] >> 10);
			words[i & 0xf] = words[i & 0xf] + s0 + words[(i - 7) & 0xf] + s1;
			sha256_round( &rdata, words[i & 0xf] );
		}
	}

	// 3. form and process terminating chunk

	memset(words, 0, 16 * sizeof(unsigned int));
	// 3.1. Break chunk into sixteen 32-bit big-endian words
	const uint8_t* last_chunk = msg + processed_chunks * 64;
	uint8_t rem_round_4 = (len & 0x3f) >> 2;
	for ( i=0; i<rem_round_4; i++)
		SHA256_BYTES_TO_UINT32( words[i], last_chunk+(i<<2) )

	// 3.2. Append the bit '1' to the message
	words[i] |= 0x80 << ((3 - (i & 3)) << 3);

	// 3. Append 0 ≤ k < 512 bits '0', thus the resulting message length (in bits) is congruent to 448 (mod 512)
	// process overflow
	if ( (len & 0x3f) > 55)
	{
		for (i = 16; i < 64; i++)
		{
			s0 = SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 7) ^ SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 18) ^ (words[(i - 15) & 0xf] >> 3);
			s1 = SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 17) ^ SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 19) ^ (words[(i - 2) & 0xf] >> 10);
			words[i & 0xf] = words[i & 0xf] + s0 + words[(i - 7) & 0xf] + s1;
		}

		for (i = 0; i < 64; i++)
			sha256_round( &rdata, words[i & 0xf] );

		memset(words, 0, 16 * sizeof(unsigned int));
	}

	// 4. Append ml, in a 64-bit big-endian integer.
	words[15] = len << 3;
	for (i = 0; i < 16; i++)
		sha256_round(&rdata, words[i]);

	for (i = 16; i < 64; i++)
	{
		s0 = SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 7) ^ SHA256_ROTATE_RIGHT(words[(i - 15) & 0xf], 18) ^ (words[(i - 15) & 0xf] >> 3);
		s1 = SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 17) ^ SHA256_ROTATE_RIGHT(words[(i - 2) & 0xf], 19) ^ (words[(i - 2) & 0xf] >> 10);
		words[i & 0xf] = words[i & 0xf] + s0 + words[(i - 7) & 0xf] + s1;
		sha256_round(&rdata, words[i & 0xf]);
	}

	// 4. get result
	SHA256_UINT32_TO_BYTES( rdata.h0, hash )
	SHA256_UINT32_TO_BYTES( rdata.h1, hash + 4 )
	SHA256_UINT32_TO_BYTES( rdata.h2, hash + 8 )
	SHA256_UINT32_TO_BYTES( rdata.h3, hash + 12 )
	SHA256_UINT32_TO_BYTES( rdata.h4, hash + 16 )
	SHA256_UINT32_TO_BYTES( rdata.h5, hash + 20 )
	SHA256_UINT32_TO_BYTES( rdata.h6, hash + 24 )
	SHA256_UINT32_TO_BYTES( rdata.h7, hash + 28 )
}

void sha_d_256(const uint8_t* msg, uint16_t len, uint8_t* hash)
{
	sha256(msg, len, hash);
	sha256(hash, 32, hash);
}

