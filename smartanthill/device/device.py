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

from copy import deepcopy
from hashlib import sha1
from json import dumps
from os import makedirs
from os.path import isdir, join
from shutil import rmtree
from tempfile import mkdtemp

from smartanthill_zc.api import ZeptoBodyPart, ZeptoProgram
from twisted.internet.defer import inlineCallbacks, returnValue
from twisted.python.constants import ValueConstant, Values

from smartanthill import FIRMWARE_VERSION
from smartanthill.cc import platformio
from smartanthill.configprocessor import ConfigProcessor
from smartanthill.device.board.base import BoardFactory
from smartanthill.exception import DeviceUnknownPlugin
from smartanthill.log import Logger
from smartanthill.network.zvd import ZeroVirtualDevice
from smartanthill.util import memoized


class DeviceStatus(Values):
    OFFLINE = ValueConstant(0)
    ONLINE = ValueConstant(1)
    WAITFORTRAINIT = ValueConstant(2)


class Device(object):

    def __init__(self, id_, options):
        self.log = Logger("device.%d" % id_)
        self.id_ = id_
        self.options = options
        self.board = BoardFactory.newBoard(options['boardId'])

    @staticmethod
    def bodyparts_to_objects(bodyparts_raw):
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
        return self.bodyparts_to_objects(self.options.get("bodyparts", []))

    def get_id(self):
        return self.id_

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

    @inlineCallbacks
    def run_program(self, program, parameters=None):
        zp = ZeptoProgram(program, self.get_bodyparts())
        opcode = zp.compile(parameters)

        zvd = ZeroVirtualDevice()
        response = yield zvd.request(self.id_, opcode)

        returnValue(zp.process_response(response))

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
        path = self.get_config_dir_by_id(self.id_)
        if not isdir(path):
            makedirs(path)
        return path

    @staticmethod
    def get_config_key_by_id(device_id):
        return "services.device.options.devices.%d" % (device_id,)

    @staticmethod
    def get_config_dir_by_id(device_id):
        return join(ConfigProcessor().get("workspace"),
                    "devices", "%d") % (device_id,)

    def is_enabled(self):
        return self.options.get("enabled", DeviceStatus.ONLINE.value)

    def get_settings_hash(self):
        settings = deepcopy(self.options)
        for key in ["name", "prevId", "enabled", "connectionUri", "firmware",
                    "status"]:
            if key in settings:
                del settings[key]
        for bodypart in settings.get("bodyparts", []):
            del bodypart['name']
        settings['currentFirmwareVersion'] = FIRMWARE_VERSION[:2]
        return sha1(dumps(settings, sort_keys=True)).hexdigest()

    def get_firmware_settings_hash(self):
        return self.options.get("firmware", {}).get("settingsHash")

    def get_status(self):
        if self.get_settings_hash() != self.get_firmware_settings_hash():
            return DeviceStatus.WAITFORTRAINIT.value
        else:
            return self.is_enabled()
