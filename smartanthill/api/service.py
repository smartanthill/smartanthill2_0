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

from twisted.python.constants import FlagConstant
from twisted.python.reflect import namedModule

from smartanthill.api.handler import APIHandlerBase
from smartanthill.configprocessor import ConfigProcessor
from smartanthill.exception import APIRequiredParams, APIUnknownRequest
from smartanthill.service import SAMultiService


class APIService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._handlers = []

    def startService(self):
        self.autoload_handlers()

        for _name, _options in self.options.items():
            if not _options.get("enabled", False):
                continue
            path = "smartanthill.api.%s" % _name
            service = namedModule(path).makeService("api.%s" % _name, _options)
            service.setServiceParent(self)

        SAMultiService.startService(self)

    def add_handler(self, handler):
        assert issubclass(handler, APIHandlerBase)
        assert handler not in self._handlers
        self._handlers.append(handler)

    def autoload_handlers(self):
        services = ConfigProcessor().get("services")
        services = sorted(services.items(), key=lambda s: s[1]['priority'])
        for (name, sopt) in services:
            if not sopt.get("enabled", False):
                continue
            try:
                handlers = namedModule(
                    "smartanthill.%s.api" % name).get_handlers()
            except (ImportError, AttributeError):
                continue

            for handler in handlers:
                if handler.KEY and handler.PERMISSION:
                    self.log.info("Auto-loaded '%s:%s' handler" % (
                        handler.PERMISSION.name, handler.KEY))
                    self.add_handler(handler)

    def request(self, action, key, data):
        assert isinstance(action, FlagConstant)
        self.log.info("Received '%s' request with key '%s' and data=%s" %
                      (action.name, key, data))
        params = data.keys()
        for hclass in self._handlers:
            hobj = hclass(action, key)
            if not hobj.match():
                continue
            elif not hobj.check_params(params):
                raise APIRequiredParams(", ".join(hobj.REQUIRED_PARAMS), key)
            return hobj.handle(data)
        raise APIUnknownRequest(action.name, key)


def makeService(name, options):
    return APIService(name, options)
