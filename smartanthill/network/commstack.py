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

from twisted.internet import protocol

from smartanthill.log import Logger
from smartanthill.network.protocol import CommStackProtocol, ControlMessage
from smartanthill.util import get_service_named


class CommStackClientFactory(protocol.ClientFactory):

    def __init__(self, device_id):
        self.name = "network.commstack.%d" % device_id
        self.log = Logger(self.name)
        self.device_id = device_id
        self._protocol = None
        self._litemq = None
        self._source_id = 0

    def buildProtocol(self, addr):
        self._protocol = CommStackProtocol()
        self._protocol.factory = self
        return self._protocol

    def startFactory(self):
        self._litemq = get_service_named("litemq")
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
            CommStackProtocol.PACKET_DIRECTION_CLIENT_TO_COMMSTACK,
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
            CommStackProtocol.PACKET_DIRECTION_DATALINK_TO_COMMSTACK,
            message
        )

    def to_datalink_callback(self, data):
        self.log.debug("Outgoing to DataLink: %s" % hexlify(data))
        self._litemq.produce(
            "network", "commstack->datalink", data, dict(binary=True))
