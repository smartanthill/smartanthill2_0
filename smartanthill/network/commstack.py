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
from struct import pack, unpack

from platformio.util import get_systype
from twisted.internet import protocol, reactor
from twisted.python.filepath import FilePath
from twisted.python.util import sibpath

from smartanthill.configprocessor import ConfigProcessor
from smartanthill.exception import NetworkCommStackServerNotExists
from smartanthill.log import Logger
from smartanthill.network.protocol import (CommStackClientProtocol,
                                           CommStackProcessProtocol,
                                           ControlMessage)
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named, load_config, singleton


class CommStackServerService(SAMultiService):

    def __init__(self, name, options):
        assert "port" in options
        SAMultiService.__init__(self, name, options)
        self._litemq = get_service_named("litemq")
        self._process = None

    def startService(self):
        p = CommStackProcessProtocol()
        p.factory = self

        self._process = reactor.spawnProcess(
            p, self.get_server_bin(),
            ["--port=%d" % self.options['port']]
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

    def buildProtocol(self, addr):
        self._protocol = CommStackClientProtocol()
        self._protocol.factory = self

        self._init_devices()

        return self._protocol

    def _init_devices(self):
        devices = get_service_named("device").get_devices()

        # initialise linked devices
        counter = 0
        for (id_, device) in devices.iteritems():
            self._protocol.send_data(
                CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_INITIALIZER,
                counter,
                self._get_device_payload(device)
            )
            counter += 1

        # finish initialisation
        self._protocol.send_data(
            CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_INITIALIZER_LAST,
            counter
        )

    def _get_device_payload(self, device):
        device_id = device.get_id()
        payload = bytearray([device_id & 0xFF, device_id >> 8])
        payload.extend(device.get_encryption_key())
        payload.append(1 if device.is_retransmitter() else 0)
        payload.append(len(device.get_buses()))  # bus_count
        payload.append(len(device.get_buses()))  # TODO bus_type_count
        for bus in device.get_buses():
            payload.append(0)  # TODO append valid bus type
        return payload

    def on_initialization_done(self, total_inited, error_code):
        assert error_code == CommStackClientProtocol.COMMLAYER_TO_CU_STATUS_OK
        assert len(get_service_named("device").get_devices()) == total_inited

    def on_status_sync_request(self, address, command, data):
        key = data[:3]
        if command == CommStackClientProtocol.REQUEST_TO_CU_READ_DATA:
            _read_data = CommStackStorage().read(key)
            payload = bytearray(
                [CommStackClientProtocol.RESPONSE_FROM_CU_READ_DATA])
            payload.extend(key)
            payload.extend(pack("<H", len(_read_data)))
            payload.extend(_read_data)
            self._protocol.send_data(
                CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_SYNC_RESPONSE,
                address,
                payload
            )
        elif command == CommStackClientProtocol.REQUEST_TO_CU_WRITE_DATA:
            _write_size = unpack("<H", data[3:5])[0]
            _write_data = (data[5:5 + _write_size]
                           if _write_size else bytearray())
            _written_size = CommStackStorage().write(key, _write_data)
            payload = bytearray(
                [CommStackClientProtocol.RESPONSE_FROM_CU_WRITE_DATA])
            payload.extend(key)
            payload.extend(pack("<H", _written_size))
            self._protocol.send_data(
                CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_SYNC_RESPONSE,
                address,
                payload
            )
        else:
            raise NotImplementedError("Sync request command %d" % command)

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
            CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_FOR_SLAVE,
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
            CommStackClientProtocol.COMMLAYER_FROM_CU_STATUS_FROM_SLAVE,
            0,  # bus_id, @TODO (0x0, 0x0)
            message
        )

    def to_hub_callback(self, data):
        data = bytearray(data)
        self.log.debug("Outgoing to Hub: %s" % hexlify(data))
        self._litemq.produce(
            "network", "commstack->hub", data, dict(binary=True))


@singleton
class CommStackStorage():

    def __init__(self):
        self.dat_file = FilePath(join(
            ConfigProcessor().get("workspace"), "commstack.dat"))

        if self.dat_file.isfile():
            self._data = load_config(self.dat_file.path)
        else:
            self._data = {}

    def __str__(self):
        return str(self._data)

    @staticmethod
    def key_to_index(key):
        assert isinstance(key, bytearray)
        assert len(key) == 3
        _key = key[:]
        _key.insert(0, 0)
        return unpack("<I", _key)[0]

    def read(self, key):
        index = self.key_to_index(key)
        if index in self._data:
            return self._data[index]
        else:
            return bytearray()

    def write(self, key, value):
        self._data[self.key_to_index(key)] = value
        return len(value)
