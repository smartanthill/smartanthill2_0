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
    return ":" in request['method']


class JSONRPCAPIProtocol(Protocol):

    def __init__(self, service):
        self._service = service

    def dataReceived(self, data):
        def write_success_response(result):
            self.transport.write(json.dumps(result))

        def write_error_response(_, message_id):
            error_response = {
                'jsonrpc': "2.0",
                'id': message_id,
                'error': SERVER_ERROR,
            }
            self.transport.write(json.dumps(error_response))

        try:
            request = json.loads(data)
        except ValueError:
            response = {
                'jsonrpc': "2.0",
                'id': None,
                'error': PARSE_ERROR
            }
            write_success_response(response)
            return

        current_message_id = None
        if isinstance(request, list):
            d = DeferredList([maybeDeferred(self.processRequest, subrequest)
                              for subrequest in request])
        else:
            current_message_id = request.get("id", None)
            d = maybeDeferred(self.processRequest, request)
        d.addCallback(write_success_response)
        d.addErrback(write_error_response, current_message_id)

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

        action_name, key = request['method'].split(":")
        try:
            action = APIPermission.lookupByName(action_name)
        except ValueError as exc:
            return error_response(exc, **INVALID_ACTION)
        data = request.get("params", {})
        d = maybeDeferred(self._service.parent.request, action, key, data)
        d.addCallbacks(success_response, error_response)
        return d


class JSONRPCAPIFactory(Factory):

    def __init__(self, service):
        self._service = service

    def buildProtocol(self, addr):
        return JSONRPCAPIProtocol(self._service)
