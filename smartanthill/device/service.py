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

from os import listdir
from os.path import join

from twisted.python.reflect import namedModule
from twisted.python.util import sibpath

from smartanthill.device.board.base import BoardBase
from smartanthill.device.device import Device
from smartanthill.device.plugin import get_plugins
from smartanthill.device.transport import get_transports
from smartanthill.exception import (BoardUnknownId, DeviceUnknownBoard,
                                    DeviceUnknownId)
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named, memoized


class DeviceService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._devices = {}

    def startService(self):
        for devid, devoptions in self.options.get("devices", {}).items():
            devid = int(devid)
            assert 0 < devid <= 255
            assert (set(["boardId", "bodyparts", "connectionUri"]) <=
                    set(devoptions.keys()))
            try:
                self._devices[devid] = Device(devid, devoptions)
            except DeviceUnknownBoard, e:
                self.log.error(e)

        SAMultiService.startService(self)

    def get_devices(self, enabled_only=True):
        if not enabled_only:
            return self._devices
        return dict((id_, device) for id_, device in self._devices.items()
                    if device.is_enabled())

    def get_device(self, id_):
        if id_ not in self._devices:
            raise DeviceUnknownId(id_)
        return self._devices[id_]

    @staticmethod
    @memoized
    def get_boards():
        boards = {}
        for d in listdir(sibpath(__file__, "board")):
            if d.startswith("__") or not d.endswith(".py"):
                continue
            module = namedModule("smartanthill.device.board.%s" % d[:-3])
            for clsname in dir(module):
                if not clsname.startswith("Board_"):
                    continue
                obj = getattr(module, clsname)()
                assert isinstance(obj, BoardBase)
                boards[obj.get_id()] = obj
        return boards

    @staticmethod
    def get_board(id_):
        try:
            return DeviceService.get_boards()[id_]
        except KeyError:
            raise BoardUnknownId(id_)

    @staticmethod
    @memoized
    def get_plugins():
        return get_plugins(
            join(get_service_named("sas").workspace_dir, "plugins"))

    @staticmethod
    @memoized
    def get_transports():
        return get_transports(
            join(get_service_named("sas").workspace_dir, "transports"))


def makeService(name, options):
    return DeviceService(name, options)
