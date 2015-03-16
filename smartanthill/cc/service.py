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

import json
from shutil import rmtree
from tempfile import mkdtemp

from twisted.application.internet import TCPServer  # pylint: disable=E0611
from twisted.application.service import MultiService
from twisted.internet.defer import maybeDeferred
from twisted.python import log, usage
from twisted.python.failure import Failure
from twisted.python.filepath import FilePath
from twisted.web import server
from twisted.web._responses import BAD_REQUEST, INTERNAL_SERVER_ERROR
from twisted.web.resource import Resource
from twisted.web.server import NOT_DONE_YET

from smartanthill import __version__
from smartanthill.cc.compiler import PlatformIOBuilder


class WebCloud(Resource):

    isLeaf = True

    def __init__(self, pioenvs_dir):
        Resource.__init__(self)
        self.pioenvs_dir = pioenvs_dir

    def render_OPTIONS(self, request):  # pylint: disable=R0201
        """ Preflighted request """
        request.setHeader("Access-Control-Allow-Origin", "*")
        request.setHeader("Access-Control-Allow-Methods",
                          "POST, OPTIONS")
        request.setHeader("Access-Control-Allow-Headers",
                          "Content-Type, Access-Control-Allow-Headers")
        return ""

    def render_POST(self, request):
        request.setHeader("Access-Control-Allow-Origin", "*")
        try:
            data = json.loads(request.content.read())
            log.msg(str(data))
            assert "pioenv" in data
            assert "defines" in data

            pb = PlatformIOBuilder(self.pioenvs_dir, data['pioenv'])
            for k, v in data['defines'].items():
                pb.append_define(k, v)

            d = maybeDeferred(pb.run)
            d.addBoth(self.delayed_render, request)
            return NOT_DONE_YET
        except:
            request.setResponseCode(BAD_REQUEST)
            return ("<html><body>The request cannot be fulfilled due to bad "
                    "syntax.</body></html>")

    def delayed_render(self, result, request):  # pylint: disable=R0201
        if isinstance(result, Failure):
            log.err(str(result.value))
            request.setResponseCode(INTERNAL_SERVER_ERROR)
            request.write(str(result.value))
        else:
            request.setHeader("Content-Type", "application/json")
            request.write(json.dumps(result))
        request.finish()


class SmartAnthillCC(MultiService):

    def __init__(self, options):
        MultiService.__init__(self)
        self.options = options

    def startService(self):
        swc = server.Site(WebCloud(self.options['pioenvsdir']))
        TCPServer(self.options['port'], swc).setServiceParent(self)
        MultiService.startService(self)

    def stopService(self):
        rmtree(self.options['pioenvsdir'])
        MultiService.stopService(self)


class Options(usage.Options):
    optParameters = [
        ["port", "p", 8130, "TCP Server port"],
        ["pioenvsdir", None, None, "The temporary path to PlatformIO ENVs "
         "directory"]
    ]

    compData = usage.Completions(
        optActions={"pioenvsdir": usage.CompleteDirs()})

    longdesc = "SmartAnthill Cloud Compiler (version %s)" % __version__

    def postOptions(self):
        if not self['pioenvsdir']:
            self['pioenvsdir'] = mkdtemp()

        pioenvsdir_path = FilePath(self['pioenvsdir'])
        if not pioenvsdir_path.exists() or not pioenvsdir_path.isdir():
            raise usage.UsageError("The path to PlatformIO ENVs directory"
                                   " is invalid")
        elif pioenvsdir_path.getPermissions().user.shorthand() != 'rwx':
            raise usage.UsageError("You don't have 'read/write/execute'"
                                   " permissions to PlatformIO ENVs directory")
        self['pioenvsdir'] = pioenvsdir_path.path


def makeService(options):
    return SmartAnthillCC(options)
