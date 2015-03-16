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
