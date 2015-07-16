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

from smartanthill.api.handler import APIHandlerBase, APIPermission
from smartanthill.util import get_service_named


def get_handlers():
    handlers = []
    device_service = get_service_named("device")
    for device in device_service.get_devices().values():
        for bodypart in device.get_bodyparts():
            required_params = \
                [field['name']
                 for field in bodypart.plugin.get_request_fields()]

            class _BodyPartAPIHandler(APIHandlerBase):
                PERMISSION = APIPermission.GET
                KEY = "device.%s.%s" % (device.id_, bodypart.get_name())
                REQUIRED_PARAMS = required_params

                def handle(self, data):
                    return 'It works!'

            handlers.append(_BodyPartAPIHandler)
    return handlers
