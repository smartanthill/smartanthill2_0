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

#if !defined __TEST_GENERATOR_H__
#define __TEST_GENERATOR_H__

// initialization
void initTestSystem();
void freeTestSystem();

// comm layer hooks
void registerIncomingPacket( const uint8_t* packet, uint16_t size );
void registerOutgoingPacket( const uint8_t* packet, uint16_t size );
bool shouldDropIncomingPacket();
bool shouldDropOutgoingPacket();
bool shouldInsertIncomingPacket( uint8_t* packet, uint16_t* size );
bool shouldInsertOutgoingPacket( uint8_t* packet, uint16_t* size );
void insertIncomingPacket();
void insertOutgoingPacket();

// sync hooks
void requestSyncExec();
void allowSyncExec();
void waitToProceed();
void justWait( uint16_t durationSec );

// scenarios
#if !defined USED_AS_MASTER
bool startSequence();
#endif


#endif // __TEST_GENERATOR_H__