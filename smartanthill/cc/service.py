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
