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
from os.path import join

from platformio.util import get_systype
from twisted.internet import protocol, reactor
from twisted.internet.error import ProcessExitedAlready

from smartanthill.log import Logger
from smartanthill.network.protocol import (CommStackClientProtocol,
                                           CommStackProcessProtocol,
                                           ControlMessage)
from smartanthill.service import SAMultiService
from smartanthill.util import get_bin_dir, get_service_named


class CommStackProcessService(SAMultiService):

    def __init__(self, name, options):
        assert set(["device_id", "port", "eeprom_path"]) <= set(options.keys())
        SAMultiService.__init__(self, name, options)
        self._litemq = get_service_named("litemq")
        self._process = None

    def startService(self):
        p = CommStackProcessProtocol()
        p.factory = self

        bin_path = join(get_bin_dir(), get_systype(), "sacommstack")
        if "windows" in get_systype():
            bin_path += ".exe"

        print bin_path

        self._process = reactor.spawnProcess(
            p, bin_path,
            ["--port=%d" % self.options['port'],
             "--psp=%s" % self.options['eeprom_path']]
        )

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            if self._process:
                self._process.loseConnection()
                try:
                    self._process.signalProcess("KILL")
                except ProcessExitedAlready:
                    pass

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def on_server_started(self, port):
        self.log.info("Server has been started on port %d" % port)
        self._litemq.produce(
            "network", "commstack.server.started",
            {"device_id": self.options['device_id'], "port": port}
        )


class CommStackClientFactory(protocol.ClientFactory):

    def __init__(self, name, device_id):
        self.name = name
        self.log = Logger(self.name)
        self.device_id = device_id
        self._litemq = get_service_named("litemq")
        self._protocol = None
        self._source_id = 0

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
            "datalink->commstack", self.from_datalink_callback
        )

    def stopFactory(self):
        for direction in ("in", "out"):
            self._litemq.unconsume("network", "%s.%s" % (self.name, direction))

    def from_client_callback(self, message, properties):
        self.log.debug("Incoming from Client: %s and properties=%s" %
                       (message, properties))
        assert isinstance(message, ControlMessage)
        self._source_id = message.source
        data = message.data
        if not isinstance(data, bytearray):
            data = bytearray(data)
        data.insert(0, 0x01)  # first packet in chain
        self._protocol.send_data(
            CommStackClientProtocol.PACKET_DIRECTION_CLIENT_TO_COMMSTACK,
            data
        )

    def to_client_callback(self, message):
        m = ControlMessage(self.device_id, self._source_id, bytearray(message))
        self.log.debug("Outgoing to Client: %s" % m)
        self._litemq.produce("network", "commstack->client", m)

    def from_datalink_callback(self, message, properties):
        self.log.debug("Incoming from DataLink: %s and properties=%s" %
                       (hexlify(message), properties))
        self._protocol.send_data(
            CommStackClientProtocol.PACKET_DIRECTION_DATALINK_TO_COMMSTACK,
            message
        )

    def to_datalink_callback(self, data):
        self.log.debug("Outgoing to DataLink: %s" % hexlify(data))
        self._litemq.produce(
            "network", "commstack->datalink", data, dict(binary=True))
