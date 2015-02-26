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

from twisted.python.reflect import namedObject

from smartanthill import __docsurl__
from smartanthill.device.operation.base import (get_operation_class,
                                                OperationType)
from smartanthill.exception import BoardUnknownOperation, DeviceUnknownBoard


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
    INFO_URL = __docsurl__

    PINS_ALIAS = None
    PINS = None
    ANALOG_PINS = None
    PWM_PINS = None

    OPERATIONS = [
        OperationType.PING,
        OperationType.LIST_OPERATIONS,
        OperationType.CONFIGURE_PIN_MODE,
        OperationType.READ_DIGITAL_PIN,
        OperationType.WRITE_DIGITAL_PIN,
        OperationType.READ_ANALOG_PIN,
        OperationType.WRITE_ANALOG_PIN,
        OperationType.CONFIGURE_ANALOG_REFERENCE
    ]

    def get_id(self):
        return self.__class__.__name__.replace("Board_", "")

    def get_name(self):
        return self.NAME if self.NAME else self.get_id()

    def get_vendor(self):
        return self.VENDOR

    def get_info_url(self):
        return self.INFO_URL

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

    def get_pinmodearg_params(self):
        raise NotImplementedError

    def get_pinanalogrefarg_params(self):
        raise NotImplementedError

    def launch_operation(self, devid, type_, data):
        try:
            assert type_ in self.OPERATIONS
            operclass = get_operation_class(type_)
        except:
            raise BoardUnknownOperation(type_.name, self.get_name())
        return operclass(self, data).launch(devid)
