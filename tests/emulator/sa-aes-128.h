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

#ifndef __SA_AES_128_H__
#define __SA_AES_128_H__

void sa_aes_print_block_16(const char* name, uint8_t* block)
{
    printf("%s", name);
    for (uint8_t i = 0; i < 16; (i)++)
    	printf("%02x ", block[i]);
    printf("\n");
}

void sa_aes_print_msg(const char* name, uint8_t* msg, uint16_t msg_sz)
{
    printf("%s", name);
    for (uint16_t i = 0; i < msg_sz; (i)++)
    	printf("%02x ", msg[i]);
    printf("\n");
}

const uint8_t rijndael_sbox[256] =
{
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
   0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
   0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
   0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
   0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
   0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
   0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
   0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
   0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
   0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
   0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

#define SA_AES_GALOIS_MUL_2( value )  ( ( (value) & 0x80) ? (((value) << 1) ^ 0x1b) : ((value) << 1) )

void sa_aes_128_nextKeyExp(uint8_t* ksc, uint8_t* rcon)
{
    ksc[0] ^= rijndael_sbox[ksc[13]] ^ *rcon;
    ksc[1] ^= rijndael_sbox[ksc[14]];
    ksc[2] ^= rijndael_sbox[ksc[15]];
    ksc[3] ^= rijndael_sbox[ksc[12]]; // RotWord

    for (uint8_t i = 4; i < 16; i += 4)
    {
        ksc[i] ^= ksc[i-4];
        ksc[i+1] ^= ksc[i-3];
        ksc[i+2] ^= ksc[i-2];
        ksc[i+3] ^= ksc[i-1];
    }

    // generate next Rcon
    *rcon = (*rcon << 1) ^ ((*rcon >> 7) & 1) * 0x1b;
}

void sa_aes_128_shiftRows(uint8_t* block)
{
	uint8_t tmp;
    // 2-nd row
    tmp = block[1];
    block[1] = block[5];
    block[5] = block[9];
    block[9] = block[13];
    block[13] = tmp;

    // 3-rd row
    tmp = block[2];
    block[2] = block[10];
    block[10] = tmp;
    tmp = block[6];
    block[6] = block[14];
    block[14] = tmp;

    // 4-th row
    tmp = block[3];
    block[3] = block[15];
    block[15] = block[11];
    block[11] = block[7];
    block[7] = tmp;
}

void sa_aes_128_mixColumns(uint8_t* block)
{
	uint8_t a, b, c, d, e;
    for (uint8_t i = 0; i < 16; i += 4)
    {
        a = block[i];
        b = block[i + 1];
        c = block[i + 2];
        d = block[i + 3];
        e = a ^ b ^ c ^ d;

        block[i] ^= e ^ SA_AES_GALOIS_MUL_2(a ^ b);
        block[i+1] ^= e ^ SA_AES_GALOIS_MUL_2(b ^ c);
        block[i+2] ^= e ^ SA_AES_GALOIS_MUL_2(c ^ d);
        block[i+3] ^= e ^ SA_AES_GALOIS_MUL_2(d ^ a);
    }
}

void sa_aes_128_encrypt_block( const uint8_t* key, const uint8_t* _block, uint8_t* res )
{
	uint8_t aes_128_buff[40];
	uint8_t block[16], ksc[32];
    memcpy( block, _block, 16 );
	memcpy( ksc, key, 16 );
	uint8_t rcon = 1;

	uint8_t i;

    for (i = 0; i < 16; (i)++) // add round key
        block[i] ^= ksc[i];

	for ( uint8_t round=0; round<9; round++ )
	{
		for (i = 0; i < 16; (i)++) // subBytes
			block[i] = rijndael_sbox[block[i]];
		sa_aes_128_shiftRows(block);
		sa_aes_128_mixColumns(block);

		sa_aes_128_nextKeyExp( ksc, &rcon );
		for (i = 0; i < 16; (i)++) // add round key
			block[i] ^= ksc[i];
	}

    for (i = 0; i < 16; (i)++) // subBytes
        block[i] = rijndael_sbox[block[i]];
	sa_aes_128_shiftRows(block);

    sa_aes_128_nextKeyExp( ksc, &rcon );
    for (i = 0; i < 16; (i)++) // add round key
        block[i] ^= ksc[i];

	memcpy( res, block, 16 );
}

#endif //__SA_AES_128_H__
