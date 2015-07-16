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

from twisted.internet.defer import DeferredList, maybeDeferred
from twisted.internet.protocol import Factory, Protocol

from smartanthill.api.handler import APIPermission

SERVER_ERROR = dict(code=-32000, message="Server error")
PARSE_ERROR = dict(code=-32700, message="Parse error")
INVALID_REQUEST = dict(code=-32600, message="Invalid Request")
INVALID_API_REQUEST = dict(code=-32001, message="Invalid API Request")
INVALID_ACTION = dict(code=-32002, message="Invalid action")


def is_valid_jsonrpc_request(request):
    return "jsonrpc" in request \
           and "2.0" == request['jsonrpc'] \
           and "method" in request \
           and "id" in request


def is_valid_api_request(request):
    return "params" in request \
           and "action" in request['params'] \
           and "data" in request['params']


class JSONRPCAPIProtocol(Protocol):

    def __init__(self, service):
        self._service = service

    def dataReceived(self, data):
        def write_response(result):
            self.transport.write(json.dumps(result))

        try:
            request = json.loads(data)
        except ValueError:
            response = {
                'jsonrpc': "2.0",
                'id': None,
                'error': PARSE_ERROR
            }
            write_response(response)
            return

        if isinstance(request, list):
            d = DeferredList([maybeDeferred(self.processRequest, subrequest)
                              for subrequest in request])
        else:
            d = maybeDeferred(self.processRequest, request)
        d.addCallback(write_response)

    def processRequest(self, request):
        response = {
            'jsonrpc': "2.0",
            'id': request.get("id", None)
        }

        if not is_valid_jsonrpc_request(request):
            response['error'] = INVALID_REQUEST
            return response
        if not is_valid_api_request(request):
            response['error'] = INVALID_API_REQUEST
            return response

        def success_response(result):
            response['result'] = result
            self._service.log.debug(response)
            return response

        def error_response(failure, code=-32000, message="Server error"):
            self._service.log.error(failure)
            response['error'] = {
                'code': code,
                'message': message,
            }
            return response

        try:
            action = APIPermission.lookupByName(request['params']['action'])
        except ValueError as exc:
            return error_response(exc, **INVALID_ACTION)
        key = request['method']
        data = request['params']['data']
        d = maybeDeferred(self._service.parent.request, action, key, data)
        d.addCallbacks(success_response, error_response)
        return d


class JSONRPCAPIFactory(Factory):

    def __init__(self, service):
        self._service = service

    def buildProtocol(self, addr):
        p = JSONRPCAPIProtocol(self._service)
        p.factory = self
        return p
