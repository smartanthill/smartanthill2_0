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

from binascii import hexlify
from os.path import exists, join

from platformio.util import get_systype
from twisted.internet import protocol, reactor
from twisted.python.util import sibpath

from smartanthill.exception import NetworkCommStackServerNotExists
from smartanthill.log import Logger
from smartanthill.network.protocol import (CommStackClientProtocol,
                                           CommStackProcessProtocol,
                                           ControlMessage)
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named


class CommStackServerService(SAMultiService):

    def __init__(self, name, options):
        assert set(["port", "eeprom_path"]) <= set(options.keys())
        SAMultiService.__init__(self, name, options)
        self._litemq = get_service_named("litemq")
        self._process = None

    def startService(self):
        p = CommStackProcessProtocol()
        p.factory = self

        self._process = reactor.spawnProcess(
            p, self.get_server_bin(),
            ["--port=%d" % self.options['port'],
             "--psp=%s" % self.options['eeprom_path']]
        )

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            result = self._process.loseConnection()
            try:
                self._process.signalProcess("KILL")
            except Exception:  # ignore any exception when can't kill process
                pass
            return result

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    @staticmethod
    def get_server_bin():
        systype = get_systype()

        if systype == "windows_amd64":
            systype = "windows_x86"
        elif systype == "linux_x86_64":
            systype = "linux_i686"
        elif systype == "linux_armv7l":
            systype = "linux_armv6l"

        bin_path = sibpath(__file__, join("bin", systype, "sacommstack"))
        if "windows" in systype:
            bin_path += ".exe"
        if not exists(bin_path):
            raise NetworkCommStackServerNotExists(bin_path)
        return bin_path

    def on_server_started(self, port):
        self.log.info("Server has been started on port %d" % port)
        self._litemq.produce(
            "network", "commstack.server.started", {"port": port})


class CommStackClientFactory(protocol.ClientFactory):

    def __init__(self, name):
        self.name = name
        self.log = Logger(self.name)
        self._litemq = get_service_named("litemq")
        self._protocol = None

    def buildProtocol(self, addr):
        self._protocol = CommStackClientProtocol()
        self._protocol.factory = self
        return self._protocol

    def startFactory(self):
        self._litemq.consume(
            "network",
            "%s.in" % self.name,
            "client->commstack",
            self.from_client_callback
        )
        self._litemq.consume(
            "network",
            "%s.out" % self.name,
            "hub->commstack", self.from_hub_callback
        )

    def stopFactory(self):
        for direction in ("in", "out"):
            self._litemq.unconsume("network", "%s.%s" % (self.name, direction))

    def from_client_callback(self, message, properties):
        self.log.debug("Incoming from Client: %s and properties=%s" %
                       (message, properties))
        assert isinstance(message, ControlMessage)
        assert isinstance(message.data, bytearray)
        data = bytearray()
        data.append(0x01)  # first packet in chain
        data.append(0x02)  # SACCP_NEW_PROGRAM
        data.extend(message.data)
        self._protocol.send_data(
            CommStackClientProtocol.PACKET_DIRECTION_CLIENT_TO_COMMSTACK,
            message.destination,
            data
        )

    def to_client_callback(self, source_id, message):
        cm = ControlMessage(source_id, 0,
                            bytearray(message[1:]))  # strip 1-st chain's byte
        self.log.debug("Outgoing to Client: %s" % cm)
        self._litemq.produce("network", "commstack->client", cm)

    def to_client_errback(self, source_id, reason):
        cm = ControlMessage(source_id, 0)
        self.log.error("Outgoing to Client: %s, error: %s" % (cm, reason))
        self._litemq.produce("network", "commstack->error", (cm, reason))

    def from_hub_callback(self, message, properties):
        self.log.debug("Incoming from Hub: %s and properties=%s" %
                       (hexlify(message), properties))
        self._protocol.send_data(
            CommStackClientProtocol.PACKET_DIRECTION_HUB_TO_COMMSTACK,
            0,  # bus_id, @TODO (0x0, 0x0)
            message
        )

    def to_hub_callback(self, data):
        data = bytearray(data)
        self.log.debug("Outgoing to Hub: %s" % hexlify(data))
        self._litemq.produce(
            "network", "commstack->hub", data, dict(binary=True))
