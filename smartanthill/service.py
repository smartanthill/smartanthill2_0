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

import sys
from os.path import expanduser, join

from twisted.application.service import MultiService, Service
from twisted.internet.defer import Deferred, maybeDeferred
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
        self.log.info(
            "Service has been started with options '%s'" % self.options)

        self._started = True
        for callback in self._onstarted:
            callback()

    def stopService(self):
        self.log.debug(
            "Going to shutdown with subservices %s" %
            [s.__class__.__name__ for s in reversed(list(self))]
        )

        def _on_stop(result):
            self.log.info("Service has been stopped")
            Service.stopService(self)
            return result

        d = Deferred()
        d.addCallback(lambda _: self.log.debug("Shutting down..."))
        for service in reversed(list(self)):
            _d = maybeDeferred(self.removeService, service)
            if not isinstance(service, MultiService):
                _d._suppressAlreadyCalled = True  # pylint: disable=W0212
            d.chainDeferred(_d)
        d.addCallback(_on_stop)
        return d

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
        self.startEnabledSubServices()
        SAMultiService.startService(self)

    def stopService(self):
        return SAMultiService.stopService(self).callback(None)

    def startEnabledSubServices(self, skip=None):
        self.startSubServices(
            [name for name, _ in self._get_ordered_service_names()
             if name not in (skip or [])])  # pylint: disable=C0325

    def stopEnabledSubServices(self, skip=None):
        return self.stopSubServices(
            [name for name, _ in reversed(self._get_ordered_service_names())
             if name not in (skip or [])])  # pylint: disable=C0325

    def startSubServices(self, names):
        if not isinstance(names, list):
            names = [names]
        for name in names:
            sopt = self.config.get("services.%s" % name)
            if not sopt.get("enabled", False):
                continue
            path = "smartanthill.%s.service" % name
            service = namedModule(path).makeService(name, sopt['options'])
            service.setServiceParent(self)

    def stopSubServices(self, names):
        if not isinstance(names, list):
            names = [names]
        d = Deferred()
        for name in names:
            sopt = self.config.get("services.%s" % name)
            if not sopt.get("enabled", False):
                continue
            d.chainDeferred(self.removeService(self.getServiceNamed(name)))
        return d

    def _get_ordered_service_names(self):
        return sorted(self.config.get("services").items(),
                      key=lambda s: s[1]['priority'])


class Options(usage.Options):
    optParameters = [["workspace", "w", join(expanduser("~"), ".smartanthill"),
                      "Path to home directory"]]

    compData = usage.Completions(
        optActions={"workspace": usage.CompleteDirs()})

    longdesc = "SmartAnthill: %s (version %s)" % (__description__, __version__)

    allowed_defconf_opts = ("logger.level", "ccurl")

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

    def opt_version(self):
        print self.longdesc
        sys.exit(0)

    def postOptions(self):
        workspace_dir = FilePath(self['workspace'])
        if not workspace_dir.exists():
            workspace_dir.makedirs()
        if workspace_dir.getPermissions().user.shorthand() != "rwx":
            raise usage.UsageError("You don't have the 'read/write/execute'"
                                   " permissions to home directory")
        self['workspace'] = workspace_dir.path


def makeService(options):
    user_options = [
        v.split("=")[0][2:] for v in sys.argv if v[:2] == "--" and "=" in v]
    result_options = dict(
        (p[0], options[p[0]]) for p in options.optParameters
        if (p[0] not in options.allowed_defconf_opts or p[0] in user_options)
    )
    return SmartAnthillService("sas", result_options)
