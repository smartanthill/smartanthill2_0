/*******************************************************************************
    Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source and compiled
    forms, with or without modification, are permitted
    provided that the following conditions are met:
        * Redistributions in source form must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in compiled form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the OLogN Technologies AG nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE
*******************************************************************************/

#if !defined __SASP_PROTOCOL_H__
#define __SASP_PROTOCOL_H__

#include "sa-common.h"
#include "sa-eeprom.h"



#define SASP_NONCE_SIZE 6
#define SASP_HEADER_SIZE SASP_NONCE_SIZE
#define SASP_ENC_BLOCK_SIZE 16
#define SASP_TAG_SIZE SASP_ENC_BLOCK_SIZE

#define DATA_SASP_SIZE (SASP_NONCE_SIZE+SASP_NONCE_SIZE+SASP_TAG_SIZE)
#define DATA_SASP_NONCE_LW_OFFSET 0 // Nonce Lower Watermark
#define DATA_SASP_NONCE_LS_OFFSET SASP_NONCE_SIZE // Nonce to use For Sending
#define DATA_SASP_LRPS_OFFSET (SASP_NONCE_SIZE+SASP_NONCE_SIZE) // Last Received Packet Signature






void SASP_initAtLifeStart( uint8_t* dataBuff )
{
	memset( dataBuff + DATA_SASP_NONCE_LW_OFFSET, 0, SASP_NONCE_SIZE );
	memset( dataBuff + DATA_SASP_NONCE_LS_OFFSET, 0, SASP_NONCE_SIZE );
	memset( dataBuff + DATA_SASP_LRPS_OFFSET, 0, SASP_TAG_SIZE );

	eeprom_write( DATA_SASP_NONCE_LW_ID, dataBuff + DATA_SASP_NONCE_LW_OFFSET, SASP_NONCE_SIZE );
	eeprom_write( DATA_SASP_NONCE_LW_ID, dataBuff + DATA_SASP_NONCE_LW_OFFSET, SASP_NONCE_SIZE );
}

void SASP_restoreFromBackup( uint8_t* dataBuff )
{
	memset( dataBuff + DATA_SASP_LRPS_OFFSET, 0, SASP_TAG_SIZE );

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
	int i;
	for ( i=0; i<SASP_NONCE_SIZE; i++ )
	{
		nonce[i] ++;
		if ( nonce[i] ) break;
	}
}

uint8_t SASP_NonceCompare( uint8_t* nonce1, uint8_t* nonce2 )
{
	if ( (nonce1[SASP_NONCE_SIZE-1]&0x7F) > (nonce2[SASP_NONCE_SIZE-1]&0x7F) ) return 1;
	if ( (nonce1[SASP_NONCE_SIZE-1]&0x7F) < (nonce2[SASP_NONCE_SIZE-1]&0x7F) ) return -1;
	int i;
	for ( i=SASP_NONCE_SIZE-2; i>=0; i-- )
	{
		if ( nonce1[i] > nonce2[i] ) return 1;
		if ( nonce1[i] < nonce2[i] ) return -1;
	}
	return 0;
}

bool SASP_NonceIsIntendedForSasp(  uint8_t* nonce )
{
	return nonce[SASP_NONCE_SIZE-1] & 0x80;
}

void SASP_NonceSetIntendedForSaspFlag(  uint8_t* nonce )
{
	nonce[SASP_NONCE_SIZE-1] |= 0x80;
}

void SASP_NonceClearForSaspFlag(  uint8_t* nonce )
{
	nonce[SASP_NONCE_SIZE-1] &= 0x7F;
}

int SASP_calcComplementarySize( int iniSize, int requiredSize )
{
	int padToRequired = requiredSize - iniSize;
	int roundUpSize = ( requiredSize / SASP_ENC_BLOCK_SIZE ) * SASP_ENC_BLOCK_SIZE + ( requiredSize % SASP_ENC_BLOCK_SIZE ? SASP_ENC_BLOCK_SIZE : 0 );
	int freeSpaceSize = roundUpSize - iniSize;
	return freeSpaceSize; 
}

int SASP_encodeSize( int size, uint8_t* buff )
{
	if ( size < 128 )
	{
		buff[ 0 ] = size;
		return 1; 
	}
	else
	{
		buff[0] = ((uint8_t)size & 0x7F) + 128;
		buff[1] = (size - 128) >> 7;
		return 2;
	}
}

int SASP_decodeSize( uint8_t* buff )
{
	if ( ( buff[ 0 ] & 128 ) == 0 )
	{
		return buff[ 0 ]; 
	}
	else
	{
		assert( (buff[1] & 0x80) == 0 );
		return 128+((uint16_t)(buff[0]&0x7F) | ((uint16_t)(buff[1]) << 7));
	}
}

int SASP_getSizeUsedForEncoding( uint8_t* buff )
{
	if ( ( buff[ 0 ] & 128 ) == 0 )
		return 1;
	else
	{
		assert( (buff[1] & 0x80) == 0 );
		return 2;
	}
}

int SASP_EncryptAndAddAuthenticationData( uint8_t first_byte, uint8_t* buffIn, int msgSize, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, const uint8_t* nonce )
{
	// msgSize covers the size of message_byte_sequence
	// (byte with MASTER_SLAVE_BIT) | nonce concatenation is used as a full nonce
	// the output is formed as follows: nonce | tag | encrypted data

	// data under encryption is as follows: first_byte | (opt) padding_size | message_byte_sequence | (opt) padding
	// (padding_size size + padding size) is encoded using SASP Encoded-Size

	// SEMI-FAKE IMPLEMENTATION 

	int i,j;
	bool singleBlock;
	int ins_pos = SASP_HEADER_SIZE + SASP_TAG_SIZE;
	int paddingAddedSize = 0;

	uint8_t c = 0; // dummy tag

	// 1. Calculate and encode complementary size; Form the first block
	int compl_size = SASP_calcComplementarySize( msgSize + 1, msgSize + 1 );
	stack[0] = first_byte;
	if ( compl_size )
		stack[0] |= 0x80;
	else
		stack[0] &= 0x7F;
	int encoding_size = SASP_encodeSize( compl_size, stack+1 );
	if ( msgSize >= SASP_ENC_BLOCK_SIZE-1-encoding_size )
	{
		singleBlock = false;
		memcpy( stack+1+encoding_size, buffIn, SASP_ENC_BLOCK_SIZE-1-encoding_size );
	}
	else
	{
		singleBlock = true;
		memcpy( stack+1+encoding_size, buffIn, msgSize );
		memset( stack+1+encoding_size+msgSize, 'x', SASP_ENC_BLOCK_SIZE-encoding_size-1-msgSize ); // add dummy padding
		paddingAddedSize = SASP_ENC_BLOCK_SIZE-encoding_size-1-msgSize;
	}
	// 1.1. do dummy encryption and calc dummy tag byte
	for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
		c ^= stack[i];
	for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
		stack[i] ^= 'y';
	memcpy( buffOut + ins_pos, stack, SASP_ENC_BLOCK_SIZE );
	ins_pos += SASP_ENC_BLOCK_SIZE;

	if ( !singleBlock )
	{
		// 2. process each next full block of the message 
		uint8_t* restOfMsg = buffIn + (SASP_ENC_BLOCK_SIZE-encoding_size-1);
		int restOfMsgSize = msgSize - (SASP_ENC_BLOCK_SIZE-encoding_size-1);
		for ( j=0; j<restOfMsgSize/SASP_ENC_BLOCK_SIZE; j++ )
		{
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[i] = restOfMsg[ SASP_ENC_BLOCK_SIZE * j + i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[i] ^= 'y';
			memcpy( buffOut + ins_pos, stack, SASP_ENC_BLOCK_SIZE );
			ins_pos += SASP_ENC_BLOCK_SIZE;
		}

		// 3. process the last incomplete block (if any)
		restOfMsg += (restOfMsgSize/SASP_ENC_BLOCK_SIZE)*SASP_ENC_BLOCK_SIZE;
		restOfMsgSize -= (restOfMsgSize/SASP_ENC_BLOCK_SIZE)*SASP_ENC_BLOCK_SIZE;
		paddingAddedSize = SASP_ENC_BLOCK_SIZE-restOfMsgSize;
		for ( i=0; i<restOfMsgSize; i++)
			stack[i] = restOfMsg[ i];
		for ( i=restOfMsgSize; i<SASP_ENC_BLOCK_SIZE; i++)
			stack[i] = 'x'; // dummy padding
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			c ^= stack[ i];
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			stack[i] ^= 'y';
		memcpy( buffOut + ins_pos, stack, SASP_ENC_BLOCK_SIZE );
		ins_pos += SASP_ENC_BLOCK_SIZE;
	}

	// 3. process forced padding (if any); note: if present, it will be a multiple of a block size
	int paddingSizeRemaining = compl_size - paddingAddedSize - 1;
	assert( paddingSizeRemaining % SASP_ENC_BLOCK_SIZE == 0 );
	for ( j=0; j<paddingSizeRemaining/SASP_ENC_BLOCK_SIZE; j++ )
	{
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			stack[i] = 'x'; // dummy padding
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			c ^= stack[ i];
		for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
			stack[i] ^= 'y';
		memcpy( buffOut + ins_pos, stack, SASP_ENC_BLOCK_SIZE );
		ins_pos += SASP_ENC_BLOCK_SIZE;
	}

	// 4. construct fake tag:
	for ( i=0; i<SASP_TAG_SIZE; i++)
		(buffOut + SASP_HEADER_SIZE)[i] = c;

	// 5. construct header:
	memcpy( buffOut, nonce, SASP_NONCE_SIZE );

	return ins_pos;
}

int SASP_IntraPacketAuthenticateAndDecrypt( uint8_t* buffIn, int msgSize, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize )
{
	// input data structure: header | tag | encrypted data
	// Therefore, msgSize is expected to be SASP_HEADER_SIZE + SASP_TAG_SIZE + k * SASP_ENC_BLOCK_SIZE
	// Structure of output buffer: first_byte | byte_sequence
	// return value: if passed, size of byte_sequence + 1
	//               if failed, -1

	if ( msgSize < SASP_HEADER_SIZE + SASP_TAG_SIZE || ( msgSize - (SASP_HEADER_SIZE + SASP_TAG_SIZE) ) % SASP_ENC_BLOCK_SIZE )
	{
//		printf( "Bad size of packet %d\n", msgSize ); // this is a special case as such packets should not arrive at this level (underlying protocol error?)
		assert( !(msgSize < SASP_HEADER_SIZE + SASP_TAG_SIZE || ( msgSize - (SASP_HEADER_SIZE + SASP_TAG_SIZE) ) % SASP_ENC_BLOCK_SIZE) );
		return -1;
	}

	int enc_data_size = msgSize - (SASP_HEADER_SIZE + SASP_TAG_SIZE);
	int byte_seq_size, byte_seq_size_remaining;
	int ins_pos = 0;
	int read_pos = SASP_HEADER_SIZE + SASP_TAG_SIZE;

	// FAKE IMPLEMENTATION 

	int i, j;
	uint8_t c = 0;
	int compl_size = 0;
	bool tagOK = true;

	// 1. decrypt the first block to find out payload size
	for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
		stack[ i ] = buffIn[read_pos + i];
	for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
		stack[ i ] ^= 'y'; // dummy decrypt
	for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
		c ^= stack[ i ]; // toward dummy tag
	read_pos += SASP_ENC_BLOCK_SIZE;

	buffOut[0] = stack[0];
	ins_pos += 1;

	if ( stack[0] & 0x80 )
		compl_size = SASP_decodeSize( stack + 1 );
	int sizeUsedForSizeEnc = SASP_getSizeUsedForEncoding( stack + 1 );
	byte_seq_size = enc_data_size - compl_size - 1;
	byte_seq_size_remaining = byte_seq_size;

	if ( byte_seq_size_remaining <= SASP_ENC_BLOCK_SIZE - 1 - sizeUsedForSizeEnc )
	{
		memcpy( buffOut + ins_pos, stack + 1 + sizeUsedForSizeEnc, byte_seq_size_remaining );
		// now we need only to complete intra-packet authentication
		for ( j=1; j<enc_data_size/SASP_ENC_BLOCK_SIZE; j++ )
		{
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] = buffIn[read_pos + i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] ^= 'y'; // dummy decrypt
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i ]; // toward dummy tag
			read_pos += SASP_ENC_BLOCK_SIZE;
		}
	}
	else
	{
		memcpy( buffOut + ins_pos, stack + 1 + sizeUsedForSizeEnc, SASP_ENC_BLOCK_SIZE - 1 - sizeUsedForSizeEnc );
		ins_pos += SASP_ENC_BLOCK_SIZE - 1 - sizeUsedForSizeEnc;
		byte_seq_size_remaining -= SASP_ENC_BLOCK_SIZE - 1 - sizeUsedForSizeEnc;

		// process full blocks related to data sequence
		for ( j=0; j<byte_seq_size_remaining/SASP_ENC_BLOCK_SIZE; j++ )
		{
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] = buffIn[read_pos + i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] ^= 'y'; // dummy decrypt
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i ]; // toward dummy tag
			read_pos += SASP_ENC_BLOCK_SIZE;
			memcpy( buffOut + ins_pos, stack, SASP_ENC_BLOCK_SIZE );
			ins_pos += SASP_ENC_BLOCK_SIZE;
		}
		byte_seq_size_remaining %= SASP_ENC_BLOCK_SIZE;

		// process block with remaining part of byte_sequence (if any)
		if ( byte_seq_size_remaining )
		{
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] = buffIn[read_pos + i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] ^= 'y'; // dummy decrypt
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i ]; // toward dummy tag
			read_pos += SASP_ENC_BLOCK_SIZE;
			memcpy( buffOut + ins_pos, stack, byte_seq_size_remaining );
			ins_pos += byte_seq_size_remaining;
		}

		// process remaining blocks (if any)
		//+++++ calc number of such blocks!!!
		for ( j=0; j<(msgSize - read_pos)/SASP_ENC_BLOCK_SIZE; j++ )
		{
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] = buffIn[read_pos + i];
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				stack[ i ] ^= 'y'; // dummy decrypt
			for ( i=0; i<SASP_ENC_BLOCK_SIZE; i++)
				c ^= stack[ i ]; // toward dummy tag
			read_pos += SASP_ENC_BLOCK_SIZE;
		}
	}

	for ( i=0; i<SASP_TAG_SIZE; i++)
		tagOK = tagOK && (buffIn + SASP_HEADER_SIZE)[i] == c;


	return tagOK ? byte_seq_size + 1 : -1;
}

int handlerSASP_send( bool repeated, uint8_t first_byte, uint8_t* buffIn, int msgSize, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data )
{
	if ( !repeated )
		SASP_NonceLS_increment( data + DATA_SASP_NONCE_LS_OFFSET );

	int retSize = SASP_EncryptAndAddAuthenticationData( first_byte, buffIn, msgSize, buffOut, buffOutSize, stack, stackSize, data+DATA_SASP_NONCE_LS_OFFSET );

	return retSize;
}

int handlerSASP_receive( uint8_t* buffIn, int msgSize, uint8_t* buffOut, int buffOutSize, uint8_t* stack, int stackSize, uint8_t* data )
{
	// 1. Perform intra-packet authentication
	int retSize = SASP_IntraPacketAuthenticateAndDecrypt( buffIn, msgSize, buffOut, buffOutSize, stack, stackSize );
	if ( retSize == -1)
		return -1;

	// 2. Is it an Error Old Nonce Message
	if ( SASP_NonceIsIntendedForSasp( buffIn ) )
	{
		// Recommended value of NLS starts from the second byte of the message; we should update NLS, if the recommended value is greater
		uint8_t nonceCmp = SASP_NonceCompare( data+DATA_SASP_NONCE_LW_OFFSET, buffOut+2 ); // skipping First Byte and reserved byte
		if ( nonceCmp < 0 )
		{
			SASP_NonceClearForSaspFlag( buffOut+1 );
			memcpy( data+DATA_SASP_NONCE_LS_OFFSET, buffOut+1, SASP_NONCE_SIZE );
			// TODO: shuold we do anything else?
		}
		return -1;
	}

	// 3. Compare nonces...
	uint8_t nonceCmp = SASP_NonceCompare( data+DATA_SASP_NONCE_LW_OFFSET, buffIn );
	if ( nonceCmp < 1 ) // error message must be prepared
	{
		uint8_t* ins_ptr = buffIn;
		*(ins_ptr++) = 0;
		memcpy( ins_ptr, data+DATA_SASP_NONCE_LW_OFFSET, SASP_NONCE_SIZE );
		ins_ptr += SASP_NONCE_SIZE;
		int retSize = SASP_EncryptAndAddAuthenticationData( 0, buffIn, msgSize, buffOut, buffOutSize, stack, stackSize, data );
		SASP_NonceSetIntendedForSaspFlag( buffOut );
		// TODO: which way to report???
		return retSize;
	}
	else if ( nonceCmp == 0 )
	{
		bool same = memcmp( data+DATA_SASP_LRPS_OFFSET, buffIn+SASP_HEADER_SIZE, SASP_TAG_SIZE );
		if (!same)
			return -1;
		// TODO: any further processing?
		// TODO: which way do we report that the packet is repeated?
		return retSize;
	}

	// 4. Finally, we have a brand new packet
	memcpy( data+DATA_SASP_NONCE_LW_OFFSET, buffIn, SASP_NONCE_SIZE ); // update Nonce Low Watermark
	memcpy( data+DATA_SASP_LRPS_OFFSET, buffIn+SASP_HEADER_SIZE, SASP_TAG_SIZE ); // save packet signature
	// TODO: which way do we report that the packet is repeated?
	return retSize;
}

#endif // __SASP_PROTOCOL_H__