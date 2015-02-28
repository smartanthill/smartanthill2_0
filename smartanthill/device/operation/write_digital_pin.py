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

from smartanthill.api.handler import APIPermission
from smartanthill.device.api import APIDeviceHandlerBase
from smartanthill.device.arg import PinArg, PinLevelArg
from smartanthill.device.operation.base import OperationBase, OperationType


class APIHandler(APIDeviceHandlerBase):

    PERMISSION = APIPermission.UPDATE
    KEY = "device.digitalpin"
    REQUIRED_PARAMS = ("devid", "pin[]")

    def handle(self, data):
        return self.launch_operation(data['devid'],
                                     OperationType.WRITE_DIGITAL_PIN, data)


class Operation(OperationBase):

    TYPE = OperationType.WRITE_DIGITAL_PIN

    def process_data(self, data):
        args = []
        for _key, _value in data.items():
            if "pin[" not in _key:
                continue
            pinarg = PinArg(*self.board.get_pinarg_params())
            pinlevelarg = PinLevelArg()
            pinarg.set_value(_key[4:-1])
            pinlevelarg.set_value(_value)
            args += [pinarg, pinlevelarg]
        return [a.get_value() for a in args]
