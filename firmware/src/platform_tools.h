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

#ifndef __PLATFORM_TOOLS_H__
#define __PLATFORM_TOOLS_H__

#ifdef ARDUINO
#include "Arduino.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void UARTInit(const uint16_t speed);
void UARTTransmitByte(uint8_t _byte);
int16_t UARTReceiveByte();
void UARTPrintln(const char *data);
uint32_t getTimeMillis();
void configurePinMode(uint8_t pinNum, uint8_t value);
uint8_t readDigitalPin(uint8_t pinNum);
void writeDigitalPin(uint8_t pinNum, uint8_t value);
void configureAnalogReference(uint8_t mode);
uint16_t readAnalogPin(uint8_t pinNum);
void writeAnalogPin(uint8_t pinNum, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
