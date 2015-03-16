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

from twisted.internet.defer import inlineCallbacks, returnValue

from smartanthill.api.handler import APIPermission
from smartanthill.device.api import APIDeviceHandlerBase
from smartanthill.device.operation.base import OperationBase, OperationType


class APIHandler(APIDeviceHandlerBase):

    PERMISSION = APIPermission.GET
    KEY = "device.operations"
    REQUIRED_PARAMS = ("devid",)

    @inlineCallbacks
    def handle(self, data):
        result = yield self.launch_operation(
            data['devid'], OperationType.LIST_OPERATIONS)
        operations = {OperationType.PING.value: OperationType.PING.name}
        for cdc in result:
            _ot = OperationType.lookupByValue(cdc)
            operations[_ot.value] = _ot.name
        returnValue(operations)


class Operation(OperationBase):

    TYPE = OperationType.LIST_OPERATIONS
