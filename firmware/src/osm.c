/**
  Copyright (c) 2015, OLogN Technologies AG. All rights reserved.

  Redistribution and use of this file in source (.rst) and compiled
  (.html, .pdf, etc.) forms, with or without modification, are permitted
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
 */

#include "osm.h"

OperationalStateMachine osm = {
    NULL,
    &_osmMakeTransition,
    &_osmUpdateState,
    &_osmFindStateByCDC
};

static const OperationalState osmStates[] =
{
#ifdef OPERTYPE_LIST_OPERATIONS
    {OPERTYPE_LIST_OPERATIONS, &osmStateListOperations},
#endif

#ifdef OPERTYPE_CONFIGURE_PIN_MODE
    {OPERTYPE_CONFIGURE_PIN_MODE, &osmStateConfigurePinMode},
#endif

#ifdef OPERTYPE_READ_DIGITAL_PIN
    {OPERTYPE_READ_DIGITAL_PIN, &osmStateReadDigitalPin},
#endif

#ifdef OPERTYPE_WRITE_DIGITAL_PIN
    {OPERTYPE_WRITE_DIGITAL_PIN, &osmStateWriteDigitalPin},
#endif

#ifdef OPERTYPE_CONFIGURE_ANALOG_REFERENCE
    {OPERTYPE_CONFIGURE_ANALOG_REFERENCE, &osmStateConfigureAnalogReference},
#endif

#ifdef OPERTYPE_READ_ANALOG_PIN
    {OPERTYPE_READ_ANALOG_PIN, &osmStateReadAnalogPin},
#endif

#ifdef OPERTYPE_WRITE_ANALOG_PIN
    {OPERTYPE_WRITE_ANALOG_PIN, &osmStateWriteAnalogPin},
#endif

    {0x0A, &osmStateAcknowledgeOutPacket}
};

static const uint8_t OSM_STATE_NUMS = sizeof osmStates / sizeof osmStates[0];

uint8_t osmGetStateNums()
{
    return OSM_STATE_NUMS;
}

uint8_t osmGetStateCDCByIndex(uint8_t index)
{
    return index < OSM_STATE_NUMS? osmStates[index].cdc : 0;
}

void _osmMakeTransition(const OperationalState* newState)
{
    osm.curState = newState;
}

void _osmUpdateState()
{
    if (osm.curState)
        osm.curState->update();
}

const OperationalState* _osmFindStateByCDC(uint8_t cdc)
{
    uint8_t i;
    for (i = 0; i < OSM_STATE_NUMS; i++)
    {
        if (osmStates[i].cdc == cdc)
            return &osmStates[i];
    }
    return IDLE_STATE;
}
