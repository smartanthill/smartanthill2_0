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

from sys import argv as sys_argv

from twisted.application.service import MultiService
from twisted.python import usage
from twisted.python.filepath import FilePath
from twisted.python.reflect import namedModule

from smartanthill import __banner__, __description__, __version__
from smartanthill.configprocessor import ConfigProcessor, get_baseconf
from smartanthill.log import Console, Logger


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


class SmartAnthillService(SAMultiService):

    INSTANCE = None

    def __init__(self, name, options):
        SmartAnthillService.INSTANCE = self
        self.workspacedir = options['workspacedir']
        self.config = ConfigProcessor(self.workspacedir, options)
        self.console = Console(100)
        self._logmessages = []
        SAMultiService.__init__(self, name, options)

    @staticmethod
    def instance():
        return SmartAnthillService.INSTANCE

    def startService(self):
        dashboard = ("http://localhost:%d/" %
                     self.config.get("services.dashboard.options.port"))
        self.log.info(__banner__.replace(
            "#wsdir#", self.workspacedir).replace("#dashboard#", dashboard))

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
    optParameters = [["workspacedir", "w", ".",
                      "The path to workspace directory"]]

    compData = usage.Completions(
        optActions={"workspacedir": usage.CompleteDirs()})

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
        wsdir_path = FilePath(self['workspacedir'])
        if not wsdir_path.exists() or not wsdir_path.isdir():
            raise usage.UsageError("The path to the workspace directory"
                                   " is invalid")
        elif wsdir_path.getPermissions().user.shorthand() != 'rwx':
            raise usage.UsageError("You don't have 'read/write/execute'"
                                   " permissions to workspace directory")
        self['workspacedir'] = wsdir_path.path


def makeService(options):
    user_options = dict((p[0], options[p[0]]) for p in options.optParameters)
    cmdopts = frozenset([v.split("=")[0][2:] for v in sys_argv
                         if v[:2] == "--" and "=" in v])
    for k in cmdopts.intersection(frozenset(user_options.keys())):
        user_options[k] = options[k]

    return SmartAnthillService("sas", user_options)
