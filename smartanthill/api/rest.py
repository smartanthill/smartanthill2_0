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

from json import dumps as json_dumps

from twisted.application.internet import TCPServer  # pylint: disable=E0611
from twisted.internet.defer import maybeDeferred
from twisted.python.failure import Failure
from twisted.python.log import NullFile
from twisted.web import server
from twisted.web.resource import NoResource, Resource
from twisted.web.server import NOT_DONE_YET

from smartanthill.api.handler import APIPermission
from smartanthill.log import Logger
from smartanthill.service import SAMultiService


class RESTSite(server.Site):

    def _openLogFile(self, path):
        log = Logger("api.rest.http")

        def wrapper(msg):
            log.debug(msg.strip())

        nf = NullFile()
        nf.write = wrapper
        return nf


class REST(Resource):

    isLeaf = True

    METHOD_TO_ACTION = dict(
        GET=APIPermission.GET,
        POST=APIPermission.ADD,
        PUT=APIPermission.UPDATE,
        DELETE=APIPermission.DELETE
    )

    def __init__(self, restservice):
        Resource.__init__(self)
        self._restservice = restservice

    def render(self, request):
        try:
            action = REST.METHOD_TO_ACTION[request.method]
            request_key = request.path[1:].replace("/", ".")
            if request_key.endswith(".json"):
                request_key = request_key[:-5]
            data = self.args_to_data(request.args)
        except Exception, e:
            self._restservice.log.error(e)
            return NoResource()

        self.args_to_data(request.args)
        d = maybeDeferred(self._restservice.parent.request,
                          action, request_key, data)
        d.addBoth(self.delayed_render, request)
        return NOT_DONE_YET

    @staticmethod
    def args_to_data(args):
        for k, v in args.items():
            if isinstance(v, list) and len(v) == 1:
                args[k] = v[0]
        return args

    def delayed_render(self, result, request):
        if isinstance(result, Failure):
            self._restservice.log.error(result)

        if request.path.endswith(".json"):
            request.setHeader("content-type", "application/json")
            request.write(self.result_to_json(result))
        elif isinstance(result, Failure):
            request.write("Error: " + self.failure_to_errmsg(result))
        else:
            request.write(str(result))
        request.finish()

    def result_to_json(self, result):
        _json = dict(status=None)
        if isinstance(result, Failure):
            _json['status'] = "error"
            _json['message'] = self.failure_to_errmsg(result)
        else:
            _json['status'] = "success"
            _json['data'] = result
        return json_dumps(_json)

    @staticmethod
    def failure_to_errmsg(failure):
        return str(failure.value)


class APIRestService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)

    def startService(self):
        TCPServer(
            self.options['port'],
            RESTSite(REST(self), logPath="/dev/null")).setServiceParent(self)
        SAMultiService.startService(self)


def makeService(name, options):
    return APIRestService(name, options)
