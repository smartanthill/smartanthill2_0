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

from os import listdir

from twisted.python.reflect import namedModule
from twisted.python.util import sibpath

from smartanthill.device.board.base import BoardBase
from smartanthill.device.device import Device
from smartanthill.exception import (BoardUnknownId, DeviceUnknownBoard,
                                    DeviceUnknownId)
from smartanthill.service import SAMultiService


class DeviceService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._devices = {}

    def startService(self):
        for devid, devoptions in self.options.get("devices", {}).items():
            devid = int(devid)
            assert 0 < devid <= 255
            assert "boardId" in devoptions
            assert "network" in devoptions

            try:
                self._devices[devid] = Device(devid, devoptions)
            except DeviceUnknownBoard, e:
                self.log.error(e)

        SAMultiService.startService(self)

    def get_devices(self):
        return self._devices

    def get_device(self, id_):
        if id_ not in self._devices:
            raise DeviceUnknownId(id_)
        return self._devices[id_]

    @staticmethod
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


def makeService(name, options):
    return DeviceService(name, options)
