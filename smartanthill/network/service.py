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

# pylint: disable=W0613

from os.path import join

from twisted.application.internet import TCPClient  # pylint: disable=E0611

from smartanthill.network.commstack import (CommStackClientFactory,
                                            CommStackServerService)
from smartanthill.network.transceiver import TransceiverService
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named


class NetworkService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._litemq = None

    def startService(self):
        self._litemq = get_service_named("litemq")
        self._litemq.declare_exchange("network")

        self._litemq.consume(
            exchange="network",
            queue="commstack.server",
            routing_key="commstack.server.started",
            callback=self.on_commstack_server_started
        )

        # initialize Communication Stack per device
        for device in get_service_named("device").get_devices().values():
            device_id = device.get_id()
            connectionUri = device.options.get("connectionUri")
            assert connectionUri

            CommStackServerService(
                "network.commstack.server.%d" % device_id,
                dict(
                    device_id=device_id,
                    port=0,  # allow system to assign free port
                    eeprom_path=join(device.get_conf_dir(), "eeprom.dat")
                )
            ).setServiceParent(self)

        # initialize transceivers
        for i, t in enumerate(self.options.get("transceivers", [])):
            if t.get("enabled", False):
                TransceiverService(
                    "network.transceiver.%d" % (i + 1), t
                ).setServiceParent(self)

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            self._litemq.unconsume("network", "commstack.server")
            self._litemq.undeclare_exchange("network")

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def on_commstack_server_started(self, message, properties):
        assert set(["device_id", "port"]) == set(message.keys())
        self.start_commstack_client(**message)

    def start_commstack_client(self, device_id, port):
        TCPClient(
            "127.0.0.1", port,
            CommStackClientFactory(
                "network.commstack.client.%d" % device_id,
                device_id
            )
        ).setServiceParent(self)


def makeService(name, options):
    return NetworkService(name, options)
