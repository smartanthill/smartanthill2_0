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

from os.path import expanduser, join
from sys import argv as sys_argv

from twisted.application.service import MultiService
from twisted.python import usage
from twisted.python.filepath import FilePath
from twisted.python.reflect import namedModule

from smartanthill import __banner__, __description__, __version__
from smartanthill.configprocessor import ConfigProcessor, get_baseconf
from smartanthill.log import Console, Logger
from smartanthill.util import singleton


class SAMultiService(MultiService):

    def __init__(self, name, options=None):
        MultiService.__init__(self)
        self.setName(name)
        self.options = options
        self.log = Logger(self.name)

        self._started = False
        self._onstarted = []

    def startService(self):
        MultiService.startService(self)

        infomsg = "Service has been started"
        self.log.info(infomsg + " with options '%s'" % self.options)

        self._started = True
        for callback in self._onstarted:
            callback()

    def stopService(self):
        MultiService.stopService(self)
        self.log.info("Service has been stopped.")

    def on_started(self, callback):
        if self._started:
            callback()
        else:
            self._onstarted.append(callback)


@singleton
class SmartAnthillService(SAMultiService):

    def __init__(self, name, options):
        self.workspace_dir = options['workspace']
        self.config = ConfigProcessor(self.workspace_dir, options)
        self.console = Console(100)
        self._logmessages = []
        SAMultiService.__init__(self, name, options)

    def startService(self):
        dashboard = ("http://localhost:%d/" %
                     self.config.get("services.dashboard.options.port"))
        self.log.info(__banner__.replace(
            "#workspace#",
            self.workspace_dir).replace("#dashboard#", dashboard))

        self.log.debug("Initial configuration: %s." % self.config)
        self._preload_subservices()
        SAMultiService.startService(self)

    def startSubService(self, name):
        sopt = self.config.get("services.%s" % name)
        if not sopt.get("enabled", False):
            return
        path = "smartanthill.%s.service" % name
        service = namedModule(path).makeService(name, sopt['options'])
        service.setServiceParent(self)

    def stopSubService(self, name):
        self.removeService(self.getServiceNamed(name))

    def restartSubService(self, name):
        self.stopSubService(name)
        self.startSubService(name)

    def _preload_subservices(self):
        services = sorted(self.config.get("services").items(), key=lambda s:
                          s[1]['priority'])
        for name, _ in services:
            self.startSubService(name)


class Options(usage.Options):
    optParameters = [["workspace", "w", join(expanduser("~"), ".smartanthill"),
                      "Path to home directory"]]

    compData = usage.Completions(
        optActions={"workspace": usage.CompleteDirs()})

    longdesc = "%s (version %s)" % (__description__, __version__)

    allowed_defconf_opts = ("logger.level",)

    def __init__(self):
        self._gather_baseparams(get_baseconf())
        usage.Options.__init__(self)

    def _gather_baseparams(self, baseconf, path=None):
        for k, v in baseconf.items():
            argname = path + "." + k if path else k
            # print argname, v, type(v)
            if isinstance(v, dict):
                self._gather_baseparams(v, argname)
            else:
                if argname not in self.allowed_defconf_opts:
                    continue
                self.optParameters.append([argname, None, v, None, type(v)])

    def postOptions(self):
        workspace_dir = FilePath(self['workspace'])
        if not workspace_dir.exists():
            workspace_dir.makedirs()
        if workspace_dir.getPermissions().user.shorthand() != "rwx":
            raise usage.UsageError("You don't have the 'read/write/execute'"
                                   " permissions to home directory")
        self['workspace'] = workspace_dir.path


def makeService(options):
    user_options = dict((p[0], options[p[0]]) for p in options.optParameters)
    cmdopts = frozenset([v.split("=")[0][2:] for v in sys_argv
                         if v[:2] == "--" and "=" in v])
    for k in cmdopts.intersection(frozenset(user_options.keys())):
        user_options[k] = options[k]

    return SmartAnthillService("sas", user_options)
