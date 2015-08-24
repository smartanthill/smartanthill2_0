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
from os import makedirs, rename
from os.path import isdir
from shutil import rmtree

from platformio.util import get_logicaldisks, get_serialports
from twisted.internet import reactor, task
from twisted.internet.defer import maybeDeferred
from twisted.python.failure import Failure
from twisted.web._responses import INTERNAL_SERVER_ERROR
from twisted.web.resource import Resource
from twisted.web.server import NOT_DONE_YET

from smartanthill.configprocessor import ConfigProcessor
from smartanthill.device.device import Device
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
        "extintPins": board.get_extint_pins()
    }
    return data


@router.add("/devices$")
def get_devices(request):
    devices = get_service_named("device").get_devices(enabled_only=False)
    data = [{
        "id": id_,
        "boardId": device.board.get_id(),
        "name": device.get_name(),
        "enabled": device.is_enabled(),
        "status": device.get_status(),
    } for id_, device in devices.iteritems()]
    return sorted(data, key=lambda d: d['id'])


@router.add("/devices/<int:devid>$")
def get_device_info(request, devid):
    assert 0 < devid <= 255
    device = get_service_named("device").get_device(devid)
    data = {
        "id": devid,
        "prevId": devid,
        "boardId": device.board.get_id(),
        "name": device.get_name(),
        "connectionUri": device.options.get("connectionUri"),
        "bodyparts": device.options.get("bodyparts"),
        "enabled": device.is_enabled(),
        "status": device.get_status(),
    }
    return data


@router.add("/devices/<int:devid>$", method="POST")
def update_device(request, devid):
    assert 0 < devid <= 255

    def _do_update(result):
        new_config = json.loads(request.content.read())
        ConfigProcessor().update(Device.get_config_key_by_id(devid),
                                 new_config)
        new_device_config_dir = Device.get_config_dir_by_id(devid)
        previous_id = new_config.get("prevId")
        if previous_id and previous_id != devid:
            ConfigProcessor().delete(Device.get_config_key_by_id(previous_id))
            old_device_config_dir = Device.get_config_key_by_id(previous_id)
            if isdir(old_device_config_dir):
                rename(old_device_config_dir, new_device_config_dir)
        if not isdir(new_device_config_dir):
            makedirs(new_device_config_dir)
        return result

    sas = get_service_named("sas")
    d = sas.stopSubServices(["api", "network", "device"])
    d.addCallback(_do_update)
    d.addCallback(lambda _: sas.startSubServices(["device", "network", "api"]))
    d.addCallback(lambda _: get_device_info(request, devid))
    return d


@router.add("/devices/<int:devid>$", method="DELETE")
def delete_device(request, devid):
    assert 0 < devid <= 255

    def _do_delete(result):
        ConfigProcessor().delete(Device.get_config_key_by_id(devid))
        device_config_dir = Device.get_config_dir_by_id(devid)
        if isdir(device_config_dir):
            rmtree(device_config_dir)
        return True

    sas = get_service_named("sas")
    d = sas.stopSubServices(["api", "network", "device"])
    d.addCallback(_do_delete)
    d.addCallback(lambda _: sas.startSubServices(["device", "network", "api"]))
    return d


@router.add("/devices/<int:devid>/buildfw")
def build_device_firmware(request, devid):
    assert 0 < devid <= 255
    device = get_service_named("device").get_device(devid)
    return maybeDeferred(device.build_firmware)


@router.add("/devices/<int:devid>/uploadfw", method="POST")
def upload_device_firmware(request, devid):
    assert 0 < devid <= 255
    sas = get_service_named("sas")

    def _on_upload_result(result):
        ConfigProcessor().update(
            Device.get_config_key_by_id(devid) + ".firmware.settingsHash",
            device_instance.get_settings_hash())

        def _on_device_restart(result):
            sas.startSubServices(["device", "network", "api"])
            return result
        return task.deferLater(reactor, 1, _on_device_restart, result)

    device_instance = get_service_named("device").get_device(devid)
    d = sas.stopSubServices(["api", "network", "device"])
    d.addCallback(lambda _, data: device_instance.upload_firmware(data),
                  json.loads(request.content.read()))
    d.addBoth(_on_upload_result)
    return d


@router.add("/plugins")
def get_plugins(request):
    plugins = []
    for p in get_service_named("device").get_plugins():
        plugins.append({
            "id": p.get_id(),
            "name": p.get_name(),
            "description": p.get_description(),
            "peripheral": p.get_peripheral(),
            "options": p.get_options(),
            "request_fields": p.get_request_fields(),
        })
    return plugins


@router.add("/serialports")
def get_serialports_route(request):
    data = get_serialports()
    for item in data:
        for k in ["hwid", "description"]:
            try:
                item[k] = item[k].encode("utf8")
                if item[k] == "n/a":
                    item[k] = None
            except UnicodeDecodeError:
                item[k] = None
    return data


@router.add("/console")
def console(request):
    return [{"date": entry[0],
             "system": entry[1],
             "level": entry[2],
             "message": entry[3]}
            for entry in get_service_named("sas").console.get_messages()]


@router.add("/settings", method="GET")
def get_settings(request):
    return ConfigProcessor().get_data()


@router.add("/settings", method="POST")
def update_settings(request):
    sas = get_service_named("sas")
    data = json.loads(request.content.read())

    def _do_update(_):
        ConfigProcessor().load_data(data)
        sas.log.set_level(ConfigProcessor().get("logger.level"))

    d = sas.stopEnabledSubServices(skip=["dashboard"])
    d.addCallback(_do_update)
    d.addCallback(lambda _: sas.startEnabledSubServices(skip=["dashboard"]))
    d.addCallback(lambda _: get_settings(request))
    return d


@router.add('/logicaldisks')
def get_logicaldisks_handler(request):
    return get_logicaldisks()


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
        # if user disconnected
        if request.finished:
            return
        request.setHeader("Access-Control-Allow-Origin", "*")
        if isinstance(result, Failure):
            self.log.error(result)
            request.setResponseCode(INTERNAL_SERVER_ERROR)
            request.write(str(result.getErrorMessage()))
        elif result:
            request.setHeader("Content-Type", "application/json")
            request.write(json.dumps(result))
        request.finish()
