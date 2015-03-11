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
