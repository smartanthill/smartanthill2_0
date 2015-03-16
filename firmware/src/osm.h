/**
  Copyright (C) 2015 OLogN Technologies AG

  This source file is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __OSM_H__
#define __OSM_H__

#include "platform_tools.h"
#include "configuration.h"

#define IDLE_STATE NULL

typedef struct
{
    uint8_t cdc; // /docs/specification/network/protocols/cdc.html
    void (*update)();
} OperationalState;

#include "osm_states.h"

typedef struct
{
    const OperationalState *curState;
    void (*makeTransition)(const OperationalState*);
    void (*updateState)();
    const OperationalState* (*findStateByCDC)(uint8_t);
} OperationalStateMachine;

extern OperationalStateMachine osm;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t osmGetStateNums();
uint8_t osmGetStateCDCByIndex(uint8_t index);
void _osmMakeTransition(const OperationalState*);
void _osmUpdateState();
const OperationalState* _osmFindStateByCDC(uint8_t);

#ifdef __cplusplus
}
#endif

#endif
