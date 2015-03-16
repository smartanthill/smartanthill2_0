# Copyright (C) 2015 OLogN Technologies AG
#
# This source file is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

"""
Channel Data Classifier
docs/specification/network/cdc/index.html
"""

from twisted.python.constants import ValueConstant, Values


class CHANNEL_URGENT(Values):

    PING = ValueConstant(0x00)
    SEGMENT_ACKNOWLEDGMENT = ValueConstant(0x0A)


class CHANNEL_BDCREQUEST(Values):

    LIST_OPERATIONS = ValueConstant(0x89)
    CONFIGURE_PIN_MODE = ValueConstant(0x8A)
    READ_DIGITAL_PIN = ValueConstant(0x8B)
    WRITE_DIGITAL_PIN = ValueConstant(0x8C)
    CONFIGURE_ANALOG_REFERENCE = ValueConstant(0x8D)
    READ_ANALOG_PIN = ValueConstant(0x8E)
    WRITE_ANALOG_PIN = ValueConstant(0x8F)
