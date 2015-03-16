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
