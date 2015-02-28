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

from os.path import join

from twisted.application.internet import TCPServer  # pylint: disable=E0611
from twisted.python.log import NullFile
from twisted.python.util import sibpath
from twisted.web import server, static

from smartanthill.dashboard.api import REST
from smartanthill.log import Logger
from smartanthill.service import SAMultiService


class DashboardSite(server.Site):

    def _openLogFile(self, path):
        log = Logger("dashboard.http")

        def wrapper(msg):
            log.debug(msg.strip())

        nf = NullFile()
        nf.write = wrapper
        return nf


class DashboardService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)

    def startService(self):
        root = static.File(sibpath(__file__, join("site", "dist")))
        root.putChild("api", REST())
        TCPServer(
            self.options['port'],
            DashboardSite(root, logPath="/dev/null")).setServiceParent(self)

        SAMultiService.startService(self)


def makeService(name, options):
    return DashboardService(name, options)
