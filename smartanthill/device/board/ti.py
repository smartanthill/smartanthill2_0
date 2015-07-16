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

# pylint: disable=W0223

from smartanthill.device.board.base import BoardBase


class BoardTIMSP430G2Base(BoardBase):

    PLATFORMIO_CONF = dict(
        platform="msp430",
        framework="energia",
        src_filter="+<*> -<hal/platforms> +<hal/platforms/wiring>",
        build_flags="-D SA_PLATFORM_WIRING"
    )

    VENDOR = "Texas Instruments"

    PINS = range(2, 16) + [18, 19]
    PINS_ALIAS = dict(
        # Labels
        P1_0=2,
        P1_1=3,
        P1_2=4,
        P1_3=5,
        P1_4=6,
        P1_5=7,
        P2_0=8,
        P2_1=9,
        P2_2=10,
        P2_3=11,
        P2_4=12,
        P2_5=13,
        P1_6=14,
        P1_7=15,
        P2_7=18,
        P2_6=19,

        # Serial
        RX=3, TX=4,

        # SPI
        SS=8, MOSI=15, MISO=14, SCK=7,

        # I2C
        SDA=14, SCL=15,

        # Analog
        A0=2, A1=3, A2=4, A3=5, A4=6, A5=7, A6=14, A7=15,

        # LEDs
        LED_BUILTIN=2,
        RED_LED=2,
        GREEN_LED=14,

        # Miscellaneous
        PUSH2=5
    )
    ANALOG_PINS = (2, 3, 4, 5, 6, 7, 14, 15)
    PWM_PINS = (4, 9, 10, 12, 13, 14, 19)
    EXTINT_PINS = range(2, 16) + [18, 19]

    def get_pinmodearg_params(self):
        return ((0, 1, 2, 4), dict(INPUT=0, OUTPUT=1, INPUT_PULLUP=2,
                                   INPUT_PULLDOWN=4))

    def get_pinanalogrefarg_params(self):
        return ((0, 1, 2, 3), dict(DEFAULT=0, EXTERNAL=1, INTERNAL1V5=2,
                                   INTERNAL2V5=3))


class Board_TI_LPmsp430g2553(BoardTIMSP430G2Base):

    NAME = "TI LaunchPad MSP430 (msp430g2553)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "lpmsp430g2553"})
        return self.PLATFORMIO_CONF
