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


class BoardStstm32Base(BoardBase):

    PLATFORMIO_CONF = dict(
        platform="ststm32",
        framework="mbed",
        src_filter="+<*> -<platforms> +<platforms/mbed>",
        build_flags="-I$PROJECTSRC_DIR/hal_common -DSA_PLATFORM_MBED"
    )

    VENDOR = "ST STM32"

    PINS_ALIAS = dict(
        # Labels
        D0="PA_3",
        D1="PA_2",
        D2="PA_10",
        D3="PB_3",
        D4="PB_5",
        D5="PB_4",
        D6="PB_10",
        D7="PA_8",
        D8="PA_9",
        D9="PC_7",
        D10="PB_6",
        D11="PA_7",
        D12="PA_6",
        D13="PA_5",
        D14="PB_9",
        D15="PB_8",

        # Serial
        RX="PA_2", TX="PA_3",

        # SPI 1
        SS_1="PB_6", MOSI_1="PA_7", MISO_1="PA_6", SCK_1="PA_5",

        # I2C 1
        SDA_1="PB_9", SCL_1="PB_8",

        # Analog
        A0="PA_0", A1="PA_1", A2="PA_4", A3="PB_0", A4="PC_1", A5="PC_0",

        # LEDs
        LED_BUILTIN="PA_5",

        # Miscellaneous
        PUSH_BUTTON="PC_13"
    )

    ANALOG_PINS = ("PA_0", "PA_1", "PA_4", "PB_0", "PC_1", "PC_0")

    PWM_PINS = ("PA_3", "PA_2", "PA_10", "PB_3", "PB_5", "PB_4", "PB_10",
                "PA_8", "PA_9", "PC_7", "PB_6", "PA_7", "PA_6", "PA_5",
                "PB_9", "PB_8")

    EXTINT_PINS = ()

    def __init__(self):
        BoardBase.__init__(self)

        self.PINS = []
        for port in ("A", "B", "C"):
            self.PINS.extend(["P%s_%d" % (port, p) for p in range(0, 16)])


class Board_Ststm32_f401re(BoardStstm32Base):

    NAME = "ST Nucleo F401RE"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({
            "board": "nucleo_f401re",
            "src_filter": ("+<*> -<platforms>"
                           "+<platforms/mbed>"
                           "-<platforms/mbed/eeprom>"
                           "+<platforms/mbed/eeprom/stm32f4xx>")
        })
        return self.PLATFORMIO_CONF


class Board_Ststm32_l152re(BoardStstm32Base):

    NAME = "ST Nucleo L152RE"

    PWM_PINS = ("PA_3", "PA_2", "PB_3", "PB_5", "PB_4", "PB_10",
                "PC_7", "PB_6", "PA_7", "PA_6", "PB_9", "PB_8")

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({
            "board": "nucleo_l152re",
            "src_filter": ("+<*> -<platforms>"
                           "+<platforms/mbed>"
                           "-<platforms/mbed/eeprom>"
                           "+<platforms/mbed/eeprom/emuram>")
        })
        return self.PLATFORMIO_CONF
