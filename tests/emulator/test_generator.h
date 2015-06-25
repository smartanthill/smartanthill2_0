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

#if !defined __TEST_GENERATOR_H__
#define __TEST_GENERATOR_H__

#include "../../firmware/src/common/sa_common.h"
#include "../../firmware/src/common/zepto_mem_mngmt.h"

// initialization
void tester_initTestSystem();
void tester_freeTestSystem();

// common testing calls
uint16_t tester_get_rand_val();

// comm layer hooks
void tester_registerIncomingPacket( REQUEST_REPLY_HANDLE mem_h );
void tester_registerOutgoingPacket( REQUEST_REPLY_HANDLE mem_h );
bool tester_shouldDropIncomingPacket();
bool tester_shouldDropOutgoingPacket();
bool tester_shouldInsertIncomingPacket( REQUEST_REPLY_HANDLE mem_h );
bool tester_shouldInsertOutgoingPacket( REQUEST_REPLY_HANDLE mem_h );

bool tester_holdOutgoingPacket( REQUEST_REPLY_HANDLE mem_h );
bool tester_isOutgoingPacketOnHold();
bool tester_releaseOutgoingPacket( REQUEST_REPLY_HANDLE mem_h );
void tester_requestHoldingPacket();
bool tester_holdPacketOnRequest( REQUEST_REPLY_HANDLE mem_h );

// sync hooks
/*void requestSyncExec();
void allowSyncExec();
void waitToProceed();
void justWaitSec( uint16_t durationSec );
void justWaitMSec( uint16_t durationMSec );*/


#endif // __TEST_GENERATOR_H__
