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

from twisted.internet.defer import inlineCallbacks, returnValue
from twisted.python.constants import ValueConstant, Values
from twisted.python.reflect import namedObject

import smartanthill.network.cdc as cdc
from smartanthill.device.arg import DeviceIDArg
from smartanthill.network.zvd import ZeroVirtualDevice


class OperationType(Values):

    PING = ValueConstant(
        cdc.CHANNEL_URGENT.PING.value)
    LIST_OPERATIONS = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.LIST_OPERATIONS.value)
    CONFIGURE_PIN_MODE = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.CONFIGURE_PIN_MODE.value)
    READ_DIGITAL_PIN = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.READ_DIGITAL_PIN.value)
    WRITE_DIGITAL_PIN = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.WRITE_DIGITAL_PIN.value)
    CONFIGURE_ANALOG_REFERENCE = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.CONFIGURE_ANALOG_REFERENCE.value)
    READ_ANALOG_PIN = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.READ_ANALOG_PIN.value)
    WRITE_ANALOG_PIN = ValueConstant(
        cdc.CHANNEL_BDCREQUEST.WRITE_ANALOG_PIN.value)


class OperationBase(object):

    TYPE = None
    TTL = 1
    ACK = True
    REQUIRED_PARAMS = None

    def __init__(self, board, data):
        self.board = board
        self.data = data

    def process_data(self, data):  # pylint: disable=W0613,R0201
        return []

    def on_result(self, result):   # pylint: disable=R0201
        return result

    @inlineCallbacks
    def launch(self, devid):
        devidarg = DeviceIDArg()
        devidarg.set_value(devid)

        zvd = ZeroVirtualDevice()
        result = yield zvd.request(self.TYPE.value, devidarg.get_value(),
                                   self.TTL, self.ACK,
                                   self.process_data(self.data))
        returnValue(self.on_result(result))

    def check_params(self, params):
        if not self.REQUIRED_PARAMS:
            return True
        params = [s if "[" not in s else s[:s.find("[")]+"[]" for s in params]
        return set(self.REQUIRED_PARAMS) <= set(params)


def get_operation_class(type_):
    assert isinstance(type_, ValueConstant)
    if not hasattr(get_operation_class, "cache"):
        get_operation_class.cache = {}
    if type_.value not in get_operation_class.cache:
        get_operation_class.cache[type_.value] = namedObject(
            "smartanthill.device.operation.%s.Operation" % type_.name.lower())
    return get_operation_class.cache[type_.value]
