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


class BoardArduinoBase(BoardBase):

    PLATFORMIO_CONF = dict(
        platform="atmelavr",
        framework="arduino",
        src_filter="+<*> -<platforms> +<platforms/wiring>",
        build_flags="-I$PROJECTSRC_DIR/hal_common -DSA_PLATFORM_WIRING"
    )

    VENDOR = "Arduino"

    PINS = range(0, 22)  # list from 0 to 21 (not 22)
    PINS_ALIAS = dict(
        # Serial
        RX=0, TX=1,

        # SPI
        SS=10, MOSI=11, MISO=12, SCK=13,

        # I2C
        SDA=18, SCL=19,

        # Analog
        A0=14, A1=15, A2=16, A3=17, A4=18, A5=19, A6=20, A7=21,

        # LEDs
        LED_BUILTIN=13
    )
    ANALOG_PINS = range(14, 22)
    PWM_PINS = (3, 5, 6, 9, 10, 11)
    EXTINT_PINS = (2, 3)


class Board_Arduino_DiecimilaATmega328(BoardArduinoBase):

    NAME = "Arduino Duemilanove or Diecimila (ATmega328)"
    ANALOG_PINS = range(14, 20)

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "diecimilaatmega328"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Fio(BoardArduinoBase):

    NAME = "Arduino Fio"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "fio"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Leonardo(BoardArduinoBase):

    NAME = "Arduino Leonardo"

    PINS = range(0, 20)
    PINS_ALIAS = dict(
        # Serial
        RX=0, TX=1,

        # I2C
        SDA=2, SCL=3,

        # Analog
        A0=14, A1=15, A2=16, A3=17, A4=18, A5=19, A6=4, A7=6, A8=8, A9=9,
        A10=10, A11=12,

        # LEDs
        LED_BUILTIN=13
    )
    ANALOG_PINS = (14, 15, 16, 17, 18, 19, 4, 6, 8, 10, 12)
    PWM_PINS = (3, 5, 6, 9, 10, 11, 13)
    EXTINT_PINS = (0, 1, 2, 3, 7)

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "leonardo"})
        return self.PLATFORMIO_CONF


class BoardArduinoMegaBase(BoardArduinoBase):

    PINS = range(0, 70)
    PINS_ALIAS = dict(
        # Serial
        RX=0, TX=1,
        RX0=0, TX0=1, RX1=19, TX1=18, RX2=17, TX2=16, RX3=15, TX3=14,

        # SPI
        SS=53, MOSI=51, MISO=50, SCK=52,

        # I2C
        SDA=20, SCL=21,

        # Analog
        A0=54, A1=55, A2=56, A3=57, A4=58, A5=59, A6=60, A7=61, A8=62, A9=63,
        A10=64, A11=65, A12=66, A13=67, A14=68, A15=69,

        # LEDs
        LED_BUILTIN=13
    )
    ANALOG_PINS = range(54, 70)
    PWM_PINS = range(2, 14) + range(44, 47)
    EXTINT_PINS = (2, 3, 18, 19, 20, 21)


class Board_Arduino_MegaATmega1280(BoardArduinoMegaBase):

    NAME = "Arduino Mega (ATmega1280)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "megaatmega1280"})
        return self.PLATFORMIO_CONF


class Board_Arduino_MegaATmega2560(BoardArduinoMegaBase):

    NAME = "Arduino Mega (ATmega2560)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "megaatmega2560"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Micro(BoardArduinoBase):

    NAME = "Arduino Micro"

    PINS = range(0, 20)
    PINS_ALIAS = dict(
        # Serial
        RX=0, TX=1,

        # I2C
        SDA=2, SCL=3,

        # Analog
        A0=14, A1=15, A2=16, A3=17, A4=18, A5=19, A6=4, A7=6, A8=8, A9=9,
        A10=10, A11=12,

        # LEDs
        LED_BUILTIN=13
    )
    ANALOG_PINS = (14, 15, 16, 17, 18, 19, 4, 6, 8, 10, 12)
    PWM_PINS = (3, 5, 6, 9, 10, 11, 13)
    EXTINT_PINS = (0, 1, 2, 3)

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "micro"})
        return self.PLATFORMIO_CONF


class Board_Arduino_MiniATmega168(BoardArduinoBase):

    NAME = "Arduino Mini (ATmega168)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "miniatmega168"})
        return self.PLATFORMIO_CONF


class Board_Arduino_MiniATmega328(BoardArduinoBase):

    NAME = "Arduino Mini (ATmega328)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "miniatmega328"})
        return self.PLATFORMIO_CONF


class Board_Arduino_NanoATmega168(BoardArduinoBase):

    NAME = "Arduino Nano (ATmega168)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "nanoatmega168"})
        return self.PLATFORMIO_CONF


class Board_Arduino_NanoATmega328(BoardArduinoBase):

    NAME = "Arduino Nano (ATmega328)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "nanoatmega328"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Pro8MHzATmega168(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega168, 3.3V, 8MHz)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "pro8MHzatmega168"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Pro16MHzATmega168(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega168, 5V, 16MHz)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "pro16MHzatmega168"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Pro8MHzATmega328(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega328, 3.3V, 8MHz)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "pro8MHzatmega328"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Pro16MHzATmega328(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega328, 5V, 16MHz)"

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "pro16MHzatmega328"})
        return self.PLATFORMIO_CONF


class Board_Arduino_Uno(BoardArduinoBase):

    NAME = "Arduino Uno"
    ANALOG_PINS = range(14, 20)

    def get_platformio_conf(self):
        self.PLATFORMIO_CONF.update({"board": "uno"})
        return self.PLATFORMIO_CONF
