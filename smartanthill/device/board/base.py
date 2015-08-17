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

from twisted.python.reflect import namedObject

from smartanthill.exception import DeviceUnknownBoard


class BoardFactory(object):  # pylint: disable=R0903

    @staticmethod
    def newBoard(name):
        vendor, _ = name.split("_")
        obj_path = "smartanthill.device.board.%s.Board_%s" % (vendor.lower(),
                                                              name)
        try:
            obj = namedObject(obj_path)()
        except AttributeError:
            raise DeviceUnknownBoard(name)
        assert isinstance(obj, BoardBase)
        return obj


class BoardBase(object):

    VENDOR = None
    NAME = None

    PINS_ALIAS = None
    PINS = None
    ANALOG_PINS = None
    PWM_PINS = None

    def get_id(self):
        return self.__class__.__name__.replace("Board_", "")

    def get_name(self):
        return self.NAME if self.NAME else self.get_id()

    def get_vendor(self):
        return self.VENDOR

    def get_pins(self):
        return self.PINS

    def get_pins_alias(self):
        return self.PINS_ALIAS

    def get_analog_pins(self):
        return self.ANALOG_PINS

    def get_pwm_pins(self):
        return self.PWM_PINS

    def get_extint_pins(self):
        return self.EXTINT_PINS

    def get_pinarg_params(self):
        return (self.PINS, self.PINS_ALIAS)

    def get_analogpinarg_params(self):
        return (self.ANALOG_PINS, self.PINS_ALIAS)

    def get_pwmpinarg_params(self):
        return (self.PWM_PINS, self.PINS_ALIAS)

    def get_platformio_conf(self):
        raise NotImplementedError

    def get_info_url(self):
        return ("http://platformio.org/#!/boards?filter%%5Btype%%5D=%s" %
                self.get_platformio_conf().get("board"))
