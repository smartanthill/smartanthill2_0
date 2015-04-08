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


#define AES_128_BLOCK_OFFSET 0
#define AES_128_RCON_OFFSET 16
#define AES_128_ROUND_OFFSET 17
#define AES_128_KEY_STATE_OFFSET 18
#define AES_128_TMP_OFFSET 34
#define AES_128_A_OFFSET 34
#define AES_128_B_OFFSET 35
#define AES_128_C_OFFSET 36
#define AES_128_D_OFFSET 37
#define AES_128_E_OFFSET 38
//#define AES_128_I_OFFSET 39
//#define AES_128_BUFF_SIZE 40
#define AES_128_BUFF_SIZE 39

uint8_t i; // TODO: consider adding to main operating block or declaring in each particular function

#define SA_AES_GALOIS_MUL_2( value )  ( ( (value) & 0x80) ? (((value) << 1) ^ 0x1b) : ((value) << 1) )

void sa_aes_128_initKeyExp(const uint8_t* key, uint8_t* aes_128_buff)
{
	for (i = 0; i < 16; (i)++)
		(aes_128_buff + AES_128_KEY_STATE_OFFSET)[i] = key[i];
	*(aes_128_buff + AES_128_RCON_OFFSET) = 1;
}

void sa_aes_128_nextKeyExp(uint8_t* aes_128_buff)
{
    (aes_128_buff + AES_128_KEY_STATE_OFFSET)[0] ^= rijndael_sbox[(aes_128_buff + AES_128_KEY_STATE_OFFSET)[13]] ^ *(aes_128_buff + AES_128_RCON_OFFSET);
    (aes_128_buff + AES_128_KEY_STATE_OFFSET)[1] ^= rijndael_sbox[(aes_128_buff + AES_128_KEY_STATE_OFFSET)[14]];
    (aes_128_buff + AES_128_KEY_STATE_OFFSET)[2] ^= rijndael_sbox[(aes_128_buff + AES_128_KEY_STATE_OFFSET)[15]];
    (aes_128_buff + AES_128_KEY_STATE_OFFSET)[3] ^= rijndael_sbox[(aes_128_buff + AES_128_KEY_STATE_OFFSET)[12]]; // RotWord

    for (i = 4; i < 16; i += 4)
    {
        (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i] ^= (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i-4];
        (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i+1] ^= (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i-3];
        (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i+2] ^= (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i-2];
        (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i+3] ^= (aes_128_buff + AES_128_KEY_STATE_OFFSET)[i-1];
    }

    // generate next Rcon
    *(aes_128_buff + AES_128_RCON_OFFSET) = (*(aes_128_buff + AES_128_RCON_OFFSET) << 1) ^ ((*(aes_128_buff + AES_128_RCON_OFFSET) >> 7) & 1) * 0x1b;
}

void sa_aes_128_addRoundKey(uint8_t keyOffset, uint8_t* aes_128_buff)
{
    for (i = 0; i < 16; (i)++)
        (aes_128_buff + AES_128_BLOCK_OFFSET)[i] ^= (aes_128_buff + AES_128_KEY_STATE_OFFSET)[keyOffset + i];
}

void sa_aes_128_subBytes(uint8_t* aes_128_buff)
{
    for (i = 0; i < 16; (i)++)
        (aes_128_buff + AES_128_BLOCK_OFFSET)[i] = rijndael_sbox[(aes_128_buff + AES_128_BLOCK_OFFSET)[i]];
}

void sa_aes_128_shiftRows(uint8_t* aes_128_buff)
{
    // 2-nd row
    *(aes_128_buff + AES_128_TMP_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[1];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[1] = (aes_128_buff + AES_128_BLOCK_OFFSET)[5];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[5] = (aes_128_buff + AES_128_BLOCK_OFFSET)[9];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[9] = (aes_128_buff + AES_128_BLOCK_OFFSET)[13];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[13] = *(aes_128_buff + AES_128_TMP_OFFSET);

    // 3-rd row
    *(aes_128_buff + AES_128_TMP_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[2];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[2] = (aes_128_buff + AES_128_BLOCK_OFFSET)[10];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[10] = *(aes_128_buff + AES_128_TMP_OFFSET);
    *(aes_128_buff + AES_128_TMP_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[6];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[6] = (aes_128_buff + AES_128_BLOCK_OFFSET)[14];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[14] = *(aes_128_buff + AES_128_TMP_OFFSET);

    // 4-th row
    *(aes_128_buff + AES_128_TMP_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[3];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[3] = (aes_128_buff + AES_128_BLOCK_OFFSET)[15];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[15] = (aes_128_buff + AES_128_BLOCK_OFFSET)[11];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[11] = (aes_128_buff + AES_128_BLOCK_OFFSET)[7];
    (aes_128_buff + AES_128_BLOCK_OFFSET)[7] = *(aes_128_buff + AES_128_TMP_OFFSET);
}

void sa_aes_128_mixColumns(uint8_t* aes_128_buff)
{
    for (i = 0; i < 16; i += 4)
    {
        *(aes_128_buff + AES_128_A_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[i];
        *(aes_128_buff + AES_128_B_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[i + 1];
        *(aes_128_buff + AES_128_C_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[i + 2];
        *(aes_128_buff + AES_128_D_OFFSET) = (aes_128_buff + AES_128_BLOCK_OFFSET)[i + 3];
        *(aes_128_buff + AES_128_E_OFFSET) = *(aes_128_buff + AES_128_A_OFFSET) ^ *(aes_128_buff + AES_128_B_OFFSET) ^ *(aes_128_buff + AES_128_C_OFFSET) ^ *(aes_128_buff + AES_128_D_OFFSET);

        (aes_128_buff + AES_128_BLOCK_OFFSET)[i] ^= *(aes_128_buff + AES_128_E_OFFSET) ^ SA_AES_GALOIS_MUL_2(*(aes_128_buff + AES_128_A_OFFSET) ^ *(aes_128_buff + AES_128_B_OFFSET));
        (aes_128_buff + AES_128_BLOCK_OFFSET)[i+1] ^= *(aes_128_buff + AES_128_E_OFFSET) ^ SA_AES_GALOIS_MUL_2(*(aes_128_buff + AES_128_B_OFFSET) ^ *(aes_128_buff + AES_128_C_OFFSET));
        (aes_128_buff + AES_128_BLOCK_OFFSET)[i+2] ^= *(aes_128_buff + AES_128_E_OFFSET) ^ SA_AES_GALOIS_MUL_2(*(aes_128_buff + AES_128_C_OFFSET) ^ *(aes_128_buff + AES_128_D_OFFSET));
        (aes_128_buff + AES_128_BLOCK_OFFSET)[i+3] ^= *(aes_128_buff + AES_128_E_OFFSET) ^ SA_AES_GALOIS_MUL_2(*(aes_128_buff + AES_128_D_OFFSET) ^ *(aes_128_buff + AES_128_A_OFFSET));
    }
}

void sa_aes_128_init(uint8_t* key, const uint8_t* message, uint8_t* aes_128_buff)
{
    for (i = 0; i < 16; (i)++)
    	(aes_128_buff + AES_128_BLOCK_OFFSET)[i] = message[i];
    sa_aes_128_initKeyExp(key, aes_128_buff);
    *(aes_128_buff + AES_128_ROUND_OFFSET) = 0;
}

void sa_aes_128_encrypt(uint8_t* aes_128_buff)
{
    sa_aes_128_addRoundKey(0, aes_128_buff);

	for ( *(aes_128_buff + AES_128_ROUND_OFFSET)=0; *(aes_128_buff + AES_128_ROUND_OFFSET)<9; (*(aes_128_buff + AES_128_ROUND_OFFSET))++ )
	{
		sa_aes_128_subBytes(aes_128_buff);
		sa_aes_128_shiftRows(aes_128_buff);
		sa_aes_128_mixColumns(aes_128_buff);

		sa_aes_128_nextKeyExp(aes_128_buff);
		sa_aes_128_addRoundKey(0, aes_128_buff);
	}

	sa_aes_128_subBytes(aes_128_buff);
	sa_aes_128_shiftRows(aes_128_buff);

    sa_aes_128_nextKeyExp(aes_128_buff);
    sa_aes_128_addRoundKey(0, aes_128_buff);
}

#endif //__SA_AES_128_H__
