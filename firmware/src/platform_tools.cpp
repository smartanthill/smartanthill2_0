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

#include "platform_tools.h"

void UARTInit(const uint16_t speed)
{
#ifdef ARDUINO
    Serial.begin(speed);
#endif
}

void UARTTransmitByte(uint8_t _byte)
{
#ifdef ARDUINO
    Serial.write(_byte);
#endif
}

int16_t UARTReceiveByte()
{
#ifdef ARDUINO
    return Serial.available()? Serial.read() : -1;
#endif
}

void UARTPrintln(const char *data)
{
#ifdef ARDUINO
    Serial.println(data);
#endif
}

uint32_t getTimeMillis()
{
#ifdef ARDUINO
    return millis();
#endif
}

void configurePinMode(uint8_t pinNum, uint8_t value)
{
#ifdef ARDUINO
    return pinMode(pinNum, value);
#endif
}

uint8_t readDigitalPin(uint8_t pinNum)
{
#ifdef ARDUINO
    return digitalRead(pinNum);
#endif
}

void writeDigitalPin(uint8_t pinNum, uint8_t value)
{
#ifdef ARDUINO
    return digitalWrite(pinNum, value);
#endif
}

void configureAnalogReference(uint8_t mode)
{
#ifdef ARDUINO
    return analogReference(mode);
#endif
}

uint16_t readAnalogPin(uint8_t pinNum)
{
#ifdef ARDUINO
    return analogRead(pinNum);
#endif
}

void writeAnalogPin(uint8_t pinNum, uint8_t value)
{
#ifdef ARDUINO
    return analogWrite(pinNum, value);
#endif
}
