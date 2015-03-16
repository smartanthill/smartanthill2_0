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

from smartanthill.api.handler import APIHandlerBase
from smartanthill.device.arg import DeviceIDArg
from smartanthill.device.operation.base import OperationType
from smartanthill.util import get_service_named


class APIDeviceHandlerBase(APIHandlerBase):  # pylint: disable=W0223

    @staticmethod
    def launch_operation(devid, type_, data=None):
        arg = DeviceIDArg()
        arg.set_value(devid)
        devid = arg.get_value()
        device = get_service_named("device")
        return device.get_device(devid).launch_operation(type_, data)


def get_handlers():
    handlers = []
    for c in OperationType.iterconstants():
        try:
            handler = namedObject(
                "smartanthill.device.operation.%s.APIHandler" % c.name.lower())
            handlers.append(handler)
        except AttributeError:
            continue
    return handlers
