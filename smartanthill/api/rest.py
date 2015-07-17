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

        request.setHeader("Access-Control-Allow-Origin", "*")
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
