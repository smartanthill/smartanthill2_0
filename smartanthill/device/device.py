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

from base64 import b64decode
from os import environ, makedirs
from os.path import dirname, isdir, join
from shutil import copyfile, rmtree
from tempfile import mkdtemp

from twisted.internet import utils
from twisted.python.util import sibpath

from smartanthill.device.board.base import BoardFactory
# from smartanthill.exception import DeviceUnknownOperation
from smartanthill.log import Logger


class Device(object):

    def __init__(self, id_, options):
        self.log = Logger("device.%d" % id_)
        self.id_ = id_
        self.options = options
        self.board = BoardFactory.newBoard(options['boardId'])

    def get_name(self):
        return self.options.get(
            "name", "Device #%d, %s" % (self.id_, self.board.get_name()))

    def run_program(self, program):
        pass
        # assert isinstance(type_, ValueConstant)
        # if type_ in self.operations:
        #     return self.board.launch_operation(self.id_, type_, data)
        # raise DeviceUnknownOperation(type_.name, self.id_)

    def build_firmware(self):
        pass

    def upload_firmware(self, firmware):
        tmp_dir = mkdtemp()
        platformioini_path = sibpath(
            dirname(__file__),
            join("cc", "embedded", "firmware", "platformio.ini")
        )

        # prepare temporary PlatformIO project
        pioenv_dir = join(tmp_dir, ".pioenvs", self.board.get_id())
        if not isdir(pioenv_dir):
            makedirs(pioenv_dir)
        copyfile(platformioini_path, join(tmp_dir, "platformio.ini"))
        with open(join(pioenv_dir, "firmware.%s" % firmware['type']),
                  "w") as f:
            f.write(b64decode(firmware['firmware']))

        defer = utils.getProcessOutputAndValue(
            "platformio",
            args=("run", "-e", self.board.get_id(), "-t", "uploadlazy",
                  "--upload-port", firmware['uploadport']),
            env=environ,
            path=tmp_dir
        )
        defer.addBoth(self._on_uploadfw_output, tmp_dir)
        return defer

    def _on_uploadfw_output(self, result, tmp_dir):
        self.log.debug(result)
        rmtree(tmp_dir)

        if result[2] != 0:  # result[2] is return code
            raise Exception(result)

        return {
            "result": "Please restart device."
        }
