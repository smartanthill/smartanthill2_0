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

/* The content of this file will be replaced by real dynamic data from main
SmartAnthill System */

#ifndef __CONFIGURATION__
#define __CONFIGURATION__

#define DEVICE_ID                            128

#define OPERTYPE_PING                        0x00
#define OPERTYPE_LIST_OPERATIONS             0x89
#define OPERTYPE_CONFIGURE_PIN_MODE          0x8A
#define OPERTYPE_READ_DIGITAL_PIN            0x8B
#define OPERTYPE_WRITE_DIGITAL_PIN           0x8C
#define OPERTYPE_CONFIGURE_ANALOG_REFERENCE  0x8D
#define OPERTYPE_READ_ANALOG_PIN             0x8E
#define OPERTYPE_WRITE_ANALOG_PIN            0x8F

#define ROUTER_UART_SPEED                    9600

#endif
