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

import json

from platformio.util import get_serialports
from twisted.internet.defer import maybeDeferred
from twisted.python.failure import Failure
from twisted.web.resource import Resource
from twisted.web.server import NOT_DONE_YET

from smartanthill.configprocessor import ConfigProcessor
from smartanthill.device.operation.base import OperationType
from smartanthill.log import Logger
from smartanthill.util import get_service_named
from smartanthill.webrouter import WebRouter

# pylint: disable=W0613

router = WebRouter(prefix="/api")


@router.add("/", method="OPTIONS")
def cors(request):
    """ Preflighted request """
    request.setHeader("Access-Control-Allow-Origin", "*")
    request.setHeader("Access-Control-Allow-Methods",
                      "GET, POST, PUT, DELETE, OPTIONS")
    request.setHeader("Access-Control-Allow-Headers",
                      "Content-Type, Access-Control-Allow-Headers")
    return None


@router.add("/boards$")
def get_boards(request):
    boards = get_service_named("device").get_boards()
    data = [{"id": id_, "name": board.get_name()} for id_, board in
            boards.iteritems()]
    return sorted(data, key=lambda d: d['id'])


@router.add("/boards/<board_id>$")
def get_board_info(request, board_id):
    board = get_service_named("device").get_board(board_id)
    data = {
        "id": board_id,
        "name": board.get_name(),
        "vendor": board.get_vendor(),
        "inforUrl": board.get_info_url(),
        "pins": board.get_pins(),
        "pinsAlias": board.get_pins_alias(),
        "analogPins": board.get_analog_pins(),
        "pwmPins": board.get_pwm_pins(),
        "extintPins": board.get_extint_pins(),
        "pinModeArgParams": board.get_pinmodearg_params()[1],
        "pinAnalogRefArgParams": board.get_pinanalogrefarg_params()[1]
    }
    return data


@router.add("/devices$")
def get_devices(request):
    devices = get_service_named("device").get_devices()
    data = [{
        "id": id_,
        "boardId": device.board.get_id(),
        "name": device.get_name()
    } for id_, device in devices.iteritems()]
    return sorted(data, key=lambda d: d['id'])


@router.add("/devices/<int:devid>$")
def get_device_info(request, devid):
    assert 0 < devid <= 255
    device = get_service_named("device").get_device(devid)
    data = {
        "id": devid,
        "boardId": device.board.get_id(),
        "name": device.get_name(),
        "operationIds": [item.value for item in device.operations],
        "network": device.options.get("network", {})
    }
    return data


@router.add("/devices/<int:devid>$", method="POST")
def update_device(request, devid):
    assert 0 < devid <= 255
    ConfigProcessor().update("services.device.options.devices.%d" % devid,
                             json.loads(request.content.read()))
    sas = get_service_named("sas")
    sas.stopSubService("network")
    sas.restartSubService("device")
    sas.startSubService("network")
    return get_device_info(request, devid)


@router.add("/devices/<int:devid>$", method="DELETE")
def delete_device(request, devid):
    assert 0 < devid <= 255
    ConfigProcessor().delete("services.device.options.devices.%d" % devid)
    sas = get_service_named("sas")
    sas.stopSubService("network")
    sas.restartSubService("device")
    sas.startSubService("network")
    return None


@router.add("/devices/<int:devid>/uploadfw", method="POST")
def uploadfw_device(request, devid):
    assert 0 < devid <= 255

    def _on_upload_result(result):
        get_service_named("sas").startSubService("network")
        return result

    get_service_named("sas").stopSubService("network")
    device = get_service_named("device").get_device(devid)
    d = maybeDeferred(device.upload_firmware,
                      json.loads(request.content.read()))
    d.addBoth(_on_upload_result)
    return d


@router.add("/operations")
def get_operations(request):
    return [{"id": item.value, "name": item.name} for item in
            OperationType.iterconstants()]


@router.add("/serialports")
def get_serialports_route(request):
    data = get_serialports()
    for item in data:
        for k in ["hwid", "description"]:
            if item[k] == "n/a":
                item[k] = None
    return data


@router.add("/console")
def console(request):
    return get_service_named("sas").console.get_messages()


class REST(Resource):

    isLeaf = True

    def __init__(self):
        Resource.__init__(self)
        self.log = Logger("dashboard.api")

    def render(self, request):
        d = maybeDeferred(router.match, request)
        d.addBoth(self.delayed_render, request)
        return NOT_DONE_YET

    def delayed_render(self, result, request):
        request.setHeader("Access-Control-Allow-Origin", "*")
        if isinstance(result, Failure):
            self.log.error(result)
            request.setResponseCode(500)
            request.write(str(result.getErrorMessage()))
        elif result:
            request.setHeader("Content-Type", "application/json")
            request.write(json.dumps(result))
        request.finish()
