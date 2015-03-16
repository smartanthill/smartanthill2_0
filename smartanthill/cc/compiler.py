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

from base64 import b64encode
from os import environ
from os.path import getsize, isdir, isfile, join

from platformio.util import get_pioenvs_dir
from twisted.internet import utils
from twisted.internet.defer import Deferred
from twisted.python import log
from twisted.python.util import sibpath

from smartanthill import FIRMWARE_VERSION


class PlatformIOBuilder(object):

    def __init__(self, pioenvs_dir, env_name):
        self.pioenvs_dir = pioenvs_dir
        self.env_name = env_name

        self._defer = Deferred()
        self._defines = []

        self.append_define("VERSION_MAJOR", FIRMWARE_VERSION[0])
        self.append_define("VERSION_MINOR", FIRMWARE_VERSION[1])

    def get_env_dir(self):
        return join(get_pioenvs_dir(), self.env_name)

    def get_firmware_path(self):
        if not isdir(self.get_env_dir()):
            return None
        for ext in ["bin", "hex"]:
            firm_path = join(self.get_env_dir(), "firmware." + ext)
            if isfile(firm_path):
                return firm_path
        return None

    def append_define(self, name, value=None):
        self._defines.append((name, value))

    def run(self):
        newenvs = dict(
            PLATFORMIO_SETTING_ENABLE_PROMPTS="false",
            PLATFORMIO_ENVS_DIR=self.pioenvs_dir,
            PLATFORMIO_SRCBUILD_FLAGS=self._get_srcbuild_flags()
        )

        d = utils.getProcessOutputAndValue(
            "platformio", args=("run", "-e", self.env_name),
            env=environ.update(newenvs),
            path=sibpath(__file__, join("embedded", "firmware"))
        )
        d.addBoth(self._on_run_callback)
        return self._defer

    def _get_srcbuild_flags(self):
        flags = []
        for d in self._defines:
            if d[1] is not None:
                flags.append("-D%s=%s" % (d[0], d[1]))
            else:
                flags.append("-D%s" % d[0])
        return " ".join(flags)

    def _on_run_callback(self, result):
        log.msg(result)

        fw_path = self.get_firmware_path()
        # result[2] is return code
        if result[2] != 0 or not fw_path or not isfile(fw_path):
            return self._defer.errback(Exception(result))

        data = dict(
            version=".".join([str(s) for s in FIRMWARE_VERSION]),
            size=getsize(fw_path),
            type=fw_path[-3:],
            firmware=b64encode(open(fw_path).read())
        )
        self._defer.callback(data)
