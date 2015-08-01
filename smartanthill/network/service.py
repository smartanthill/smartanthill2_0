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

from twisted.application.internet import TCPClient  # pylint: disable=E0611
from twisted.internet import reactor
from twisted.internet.serialport import SerialPort

from smartanthill import exception
from smartanthill.network.commstack import (CommStackClientFactory,
                                            CommStackProcessService)
from smartanthill.network.protocol import DataLinkProtocol
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named


class DataLinkService(SAMultiService):

    RECONNECT_DELAY = 1  # in seconds

    def __init__(self, name, options):
        assert "connection" in options
        assert isinstance(options['connection'], ConnectionInfo)
        SAMultiService.__init__(self, name, options)

        self._protocol = DataLinkProtocol()
        self._protocol.factory = self

        self._link = None
        self._litemq = None
        self._reconnect_nums = 0
        self._reconnect_callid = None

    def startService(self):
        connection = self.options['connection']
        try:
            self._link = self._make_link(connection)
        except OSError as e:
            self.log.error(str(e))
            self.log.error(
                exception.NetworkDataLinkConnectionFailure(self.options))
            self._reconnect_nums += 1
            self._reconnect_callid = reactor.callLater(
                self._reconnect_nums * self.RECONNECT_DELAY, self.startService)
            return

        self._litemq = get_service_named("litemq")
        self._litemq.consume(
            exchange="network",
            queue=self.name,
            routing_key="commstack->datalink",
            callback=self.out_packet_callback
        )

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            if self._reconnect_callid:
                self._reconnect_callid.cancel()
            if self._link:
                self._link.loseConnection()
            if self._litemq:
                self._litemq.unconsume("network", self.name)

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def _make_link(self, connection):
        assert isinstance(connection, ConnectionInfo)
        protocol_type = connection.get_protocol()
        if protocol_type not in ("serial",):
            raise exception.NetworkDataLinkUnsupportedProtocol(protocol_type)
        if protocol_type == "serial":
            return self._make_serial_link(connection)

    def _make_serial_link(self, connection):
        kwargs = connection.get_options()
        kwargs['deviceNameOrPortNumber'] = connection.get_address()
        kwargs['protocol'] = self._protocol
        kwargs['reactor'] = reactor
        return SerialPort(**kwargs)

    def in_packet_callback(self, packet):
        self.log.debug("Received incoming packet %s" % hexlify(packet))
        self._litemq.produce(
            "network", "datalink->commstack", packet, dict(binary=True))

    def out_packet_callback(self, message, properties):
        self.log.debug("Received outgoing packet %s" % hexlify(message))
        self._protocol.send_packet(message)


class ConnectionInfo(object):

    def __init__(self, uri):
        assert "://" in uri
        self.uri = uri

        self._protocol = None
        self._address = None
        self._options = {}

        self._parse_uri(uri)

    def __repr__(self):
        return "ConnectionInfo: %s" % self.uri

    def get_uri(self):
        return self.uri

    def get_protocol(self):
        return self._protocol

    def get_address(self):
        return self._address

    def get_options(self):
        return self._options

    def _parse_uri(self, uri):
        protoend_pos = uri.index("://")
        options_pos = uri.find("?")

        self._protocol = uri[:uri.index("://")]

        if options_pos != -1:
            self._address = uri[protoend_pos + 3:options_pos]
        else:
            self._address = uri[protoend_pos + 3:]

        if options_pos != -1:
            for option in uri[options_pos + 1:].split("&"):
                key, value = option.split("=")
                self._options[key] = value


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

        for device in get_service_named("device").get_devices().values():
            device_id = device.get_id()
            connectionUri = device.options.get("connectionUri")
            assert connectionUri

            CommStackProcessService(
                "network.commstack.server.%d" % device_id,
                dict(
                    device_id=device_id,
                    port=0,  # allow system to assign free port
                    eeprom_path=join(device.get_conf_dir(), "eeprom.dat")
                )
            ).setServiceParent(self)

            # SADLP
            DataLinkService(
                "network.datalink.%d" % device_id,
                dict(connection=ConnectionInfo(connectionUri))
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
            "", port,
            CommStackClientFactory(
                "network.commstack.client.%d" % device_id,
                device_id
            )
        ).setServiceParent(self)


def makeService(name, options):
    return NetworkService(name, options)
