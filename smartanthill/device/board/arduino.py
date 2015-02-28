# Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
#
# Redistribution and use of this file in source and compiled
# forms, with or without modification, are permitted
# provided that the following conditions are met:
#     * Redistributions in source form must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in compiled form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the OLogN Technologies AG nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE

from smartanthill.device.board.base import BoardBase


class BoardArduinoBase(BoardBase):

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

    def get_pinmodearg_params(self):
        return ((0, 1, 2), dict(INPUT=0, OUTPUT=1, INPUT_PULLUP=2))

    def get_pinanalogrefarg_params(self):
        return ((0, 1, 2), dict(DEFAULT=0, EXTERNAL=1, INTERNAL=2))


class Board_Arduino_DiecimilaATmega168(BoardArduinoBase):

    NAME = "Arduino Duemilanove or Diecimila (ATmega168)"
    ANALOG_PINS = range(14, 20)


class Board_Arduino_DiecimilaATmega328(BoardArduinoBase):

    NAME = "Arduino Duemilanove or Diecimila (ATmega328)"
    ANALOG_PINS = range(14, 20)


class Board_Arduino_Fio(BoardArduinoBase):

    NAME = "Arduino Fio"


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
    ANALOG_PINS = (14, 15, 16, 17, 18, 19, 4, 6, 8, 8, 10, 12)
    PWM_PINS = (3, 5, 6, 9, 10, 11, 13)
    EXTINT_PINS = (0, 1, 2, 3, 7)


class Board_Arduino_LilyPadUSB(BoardArduinoBase):

    NAME = "Arduino LilyPad USB"


class Board_Arduino_LilyPadATmega168(BoardArduinoBase):

    NAME = "Arduino LilyPad (ATmega168)"


class Board_Arduino_LilyPadATmega328(BoardArduinoBase):

    NAME = "Arduino LilyPad (ATmega328)"


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


class Board_Arduino_MegaATmega2560(BoardArduinoMegaBase):

    NAME = "Arduino Mega (ATmega2560)"


class Board_Arduino_MegaADK(BoardArduinoMegaBase):

    NAME = "Arduino Mega ADK"


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
    ANALOG_PINS = (14, 15, 16, 17, 18, 19, 4, 6, 8, 8, 10, 12)
    PWM_PINS = (3, 5, 6, 9, 10, 11, 13)
    EXTINT_PINS = (0, 1, 2, 3)


class Board_Arduino_MiniATmega168(BoardArduinoBase):

    NAME = "Arduino Mini (ATmega168)"


class Board_Arduino_MiniATmega328(BoardArduinoBase):

    NAME = "Arduino Mini (ATmega328)"


class Board_Arduino_NanoATmega168(BoardArduinoBase):

    NAME = "Arduino Nano (ATmega168)"


class Board_Arduino_NanoATmega328(BoardArduinoBase):

    NAME = "Arduino Nano (ATmega328)"


class Board_Arduino_Pro8MHzATmega168(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega168, 3.3V, 8MHz)"


class Board_Arduino_Pro16MHzATmega168(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega168, 5V, 16MHz)"


class Board_Arduino_Pro8MHzATmega328(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega328, 3.3V, 8MHz)"


class Board_Arduino_Pro16MHzATmega328(BoardArduinoBase):

    NAME = "Arduino Pro or Pro Mini (ATmega328, 5V, 16MHz)"


class Board_Arduino_Uno(BoardArduinoBase):

    NAME = "Arduino Uno"
    ANALOG_PINS = range(14, 20)
