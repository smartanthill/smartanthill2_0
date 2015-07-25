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

import os.path
from shutil import rmtree
from tempfile import mkdtemp

from smartanthill_zc.api import ZeptoBodyPart, ZeptoProgram
from twisted.internet.defer import succeed

from smartanthill.cc import platformio
from smartanthill.configprocessor import ConfigProcessor
from smartanthill.device.board.base import BoardFactory
from smartanthill.exception import DeviceUnknownPlugin
from smartanthill.log import Logger
from smartanthill.util import memoized

DEVICE_CONFIG_KEY_FORMAT = "services.device.options.devices.%d"
DEVICE_CONFIG_DIR_FORMAT = os.path.join(ConfigProcessor().get("workspace"),
                                        "devices", "%d")


class Device(object):

    def __init__(self, id_, options):
        self.log = Logger("device.%d" % id_)
        self.id_ = id_
        self.options = options
        self.board = BoardFactory.newBoard(options['boardId'])

    @staticmethod
    def bodypartsToObjects(bodyparts_raw):
        from smartanthill.device.service import DeviceService

        def _get_plugin(id_):
            for item in DeviceService.get_plugins():
                if item.get_id() == id_:
                    return item
            raise DeviceUnknownPlugin(id_)

        bodyparts = []
        for id_, item in enumerate(bodyparts_raw):
            bodyparts.append(ZeptoBodyPart(
                _get_plugin(item['pluginId']),
                id_,
                item['name'],
                item.get("peripheral", None),
                item.get("options", None)
            ))
        return bodyparts

    @memoized
    def get_bodyparts(self):
        return self.bodypartsToObjects(self.options.get("bodyparts", []))

    def get_name(self):
        return self.options.get(
            "name", "Device #%d, %s" % (self.id_, self.board.get_name()))

    def execute_bodypart(self, name, request_fields):
        bodypart = None
        for bodypart in self.get_bodyparts():
            if bodypart.get_name() == name:
                break
        assert isinstance(bodypart, ZeptoBodyPart)

        program = "return %s.Execute(%s);" % (name, ", ".join([
            f['name'] for f in bodypart.plugin.get_request_fields()
        ]))
        return self.run_program(program, request_fields)

    def run_program(self, program, parameters=None):  # pylint: disable=R0201
        zp = ZeptoProgram(program, self.get_bodyparts())
        opcode = zp.compile(parameters)
        return succeed(program + str(opcode))
        # assert isinstance(type_, ValueConstant)
        # if type_ in self.operations:
        #     return self.board.launch_operation(self.id_, type_, data)
        # raise DeviceUnknownOperation(type_.name, self.id_)

    def build_firmware(self):
        def _on_result(result, project_dir):
            rmtree(project_dir)
            return result

        project_dir = mkdtemp()
        d = platformio.build_firmware(
            project_dir,
            self.board.get_platformio_conf(),
            self.get_bodyparts()
        )
        d.addBoth(_on_result, project_dir)
        return d

    def upload_firmware(self, data):
        def _on_result(result, project_dir):
            rmtree(project_dir)
            return result

        project_dir = mkdtemp()
        d = platformio.upload_firmware(
            project_dir,
            self.board.get_platformio_conf(),
            data
        )
        d.addBoth(_on_result, project_dir)
        return d

    def get_conf_dir(self):
        return DEVICE_CONFIG_DIR_FORMAT % self.id_
