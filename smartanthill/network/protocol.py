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

from binascii import hexlify
from struct import pack, unpack

from twisted.internet import protocol
from twisted.protocols.basic import LineReceiver

from smartanthill.exception import (NetworkCommStackServerInternalError,
                                    SABaseException)


class ControlMessage(object):

    def __init__(self, source, destination, data=None):
        self.source = source
        self.destination = destination
        self.data = data or bytearray()
        assert isinstance(data, bytearray)

        assert 0 <= self.source <= 65535 and 0 <= self.destination <= 65535
        assert self.source != self.destination

    def __repr__(self):
        return ("ControlMessage: source=%d, destination=%d, data=%s" % (
            self.source, self.destination,
            hexlify(self.data if self.data else "")))

    def __eq__(self, other):
        for attr in ("source", "destination", "data"):
            if getattr(self, attr) != getattr(other, attr):
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


class CommStackProcessProtocol(protocol.ProcessProtocol):

    def __init__(self):
        self._line_parser = LineReceiver()
        self._line_parser.delimiter = "\n"
        self._line_parser.makeConnection(None)
        self._line_parser.lineReceived = self.outLineReceived

        self._port_is_found = False

    def connectionMade(self):
        self.factory.log.debug("Child process id = %s" % self.transport.pid)

    def outReceived(self, data):
        self._line_parser.dataReceived(data)

    def outLineReceived(self, line):
        line = line.strip()
        self.factory.log.debug(line)
        if not self._port_is_found:
            self._parse_server_port(line)

    def errReceived(self, data):
        self.factory.log.error(data)

    def processExited(self, reason):
        self.factory.log.debug("Process has exited")

    def processEnded(self, reason):
        self.factory.log.debug("Process has ended")
        self.factory.stopService()

    def _parse_server_port(self, line):
        if "socket: started on port" not in line:
            return
        port = int(line.split()[-1])
        assert port > 0
        self._port_is_found = True
        self.factory.on_server_started(port)


class CommStackClientProtocol(protocol.Protocol):

    #
    # Constants described below should be synced with
    # https://github.com/smartanthill/commstack-server/blob/develop/server/
    # src/commstack_commlayer.h#L39
    #

    # received packet status
    COMMLAYER_FROM_CU_STATUS_FAILED = 0
    COMMLAYER_FROM_CU_STATUS_FOR_SLAVE = 38
    COMMLAYER_FROM_CU_STATUS_FROM_SLAVE = 40
    COMMLAYER_FROM_CU_STATUS_INITIALIZER = 50
    COMMLAYER_FROM_CU_STATUS_INITIALIZER_LAST = 51
    COMMLAYER_FROM_CU_STATUS_ADD_DEVICE = 55
    COMMLAYER_FROM_CU_STATUS_REMOVE_DEVICE = 60
    COMMLAYER_FROM_CU_STATUS_SYNC_RESPONSE = 57
    COMMLAYER_FROM_CU_STATUS_GET_DEV_PERF_COUNTERS_REQUEST = 70

    # sent packet status
    COMMLAYER_TO_CU_STATUS_RESERVED_FAILED = 0
    COMMLAYER_TO_CU_STATUS_FOR_SLAVE = 37
    COMMLAYER_TO_CU_STATUS_FROM_SLAVE = 35
    COMMLAYER_TO_CU_STATUS_SLAVE_ERROR = 47
    COMMLAYER_TO_CU_STATUS_SYNC_REQUEST = 56
    COMMLAYER_TO_CU_STATUS_INITIALIZATION_DONE = 60
    COMMLAYER_TO_CU_STATUS_DEVICE_ADDED = 61
    COMMLAYER_TO_CU_STATUS_DEVICE_REMOVED = 62
    COMMLAYER_FROM_CU_STATUS_GET_DEV_PERF_COUNTERS_REPLY = 70

    # REQUEST/REPLY CODES
    REQUEST_TO_CU_WRITE_DATA = 0
    REQUEST_TO_CU_READ_DATA = 1
    RESPONSE_FROM_CU_WRITE_DATA = 0
    RESPONSE_FROM_CU_READ_DATA = 1

    # error codes
    COMMLAYER_TO_CU_STATUS_OK = 0
    COMMLAYER_TO_CU_STATUS_FAILED_EXISTS = 1
    COMMLAYER_TO_CU_STATUS_FAILED_INCOMPLETE_OR_CORRUPTED_DATA = 2
    COMMLAYER_TO_CU_STATUS_FAILED_UNKNOWN_REASON = 3
    COMMLAYER_TO_CU_STATUS_FAILED_UNEXPECTED_PACKET = 4
    COMMLAYER_TO_CU_STATUS_FAILED_DOES_NOT_EXIST = 5

    def __init__(self):
        self._buffer = bytearray()
        self._queue = []

    def connectionMade(self):
        for item in self._queue:
            self.send_data(*item)

    def send_data(self, status, address, payload=None):
        if not self.connected:
            self._queue.append((status, address, payload))
            return

        if payload is None:
            payload = bytearray()

        assert isinstance(payload, bytearray)
        size = len(payload)
        packet = bytearray(pack("<HHB", size, address, status))
        packet.extend(payload)

        self.transport.write(str(packet))
        self.factory.log.debug(
            "Sent packet: size=%d, address=%d, status=%d, payload=%s" % (
                size, address, status, hexlify(payload))
        )

    def dataReceived(self, data):
        """
        Packet structure
        | size of payload (2 bytes, low, high)
        | address (2 bytes, low, high)
        | status (1 byte)
        | payload (variable size)
        """

        self.factory.log.debug("Received data %s" % hexlify(data))
        self._buffer.extend(bytearray(data))

        while len(self._buffer) >= 5:  # min size of packet
            size, address, status = unpack("<HHB", self._buffer[:5])
            payload = self._buffer[5:5 + size] if size else []
            del self._buffer[:5 + size]

            self.factory.log.debug(
                "Received packet: size=%d, address=%d, status=%d, "
                "payload=%s" % (
                    size, address, status, hexlify(payload))
            )
            self._process_packet(status, address, payload)

    def _process_packet(self, status, address, payload):
        if status == self.COMMLAYER_TO_CU_STATUS_INITIALIZATION_DONE:
            return self.factory.on_initialization_done(
                address, payload[0]
            )

        elif status == self.COMMLAYER_TO_CU_STATUS_SYNC_REQUEST:
            return self.factory.on_status_sync_request(
                address, payload[0], payload[1:])

        elif status == self.COMMLAYER_TO_CU_STATUS_FOR_SLAVE:
            return self.factory.to_hub_callback(payload)

        elif status == self.COMMLAYER_TO_CU_STATUS_FROM_SLAVE:
            return self.factory.to_client_callback(
                address, payload)

        elif status == self.COMMLAYER_TO_CU_STATUS_SLAVE_ERROR:
            return self.factory.to_client_errback(
                address,
                NetworkCommStackServerInternalError())
        raise SABaseException("Invalid status %d" % status)


class DataLinkProtocol(protocol.Protocol):

    FRAGMENT_START_CODE = 0x01
    FRAGMENT_ESCAPE_CODE = 0xff
    FRAGMENT_END_CODE = 0x17

    BUFFER_MAX_LEN = 300

    def __init__(self):
        self._buffer = bytearray()

    def packet_to_frame(self, packet):
        assert isinstance(packet, bytearray)
        frame = bytearray([self.FRAGMENT_START_CODE])
        for byte in packet:
            if byte == self.FRAGMENT_START_CODE:
                frame.extend([self.FRAGMENT_ESCAPE_CODE, 0x00])
            elif byte == self.FRAGMENT_END_CODE:
                frame.extend([self.FRAGMENT_ESCAPE_CODE, 0x02])
            elif byte == self.FRAGMENT_ESCAPE_CODE:
                frame.extend([self.FRAGMENT_ESCAPE_CODE, 0x03])
            else:
                frame.append(byte)
        frame.append(self.FRAGMENT_END_CODE)
        return frame

    def frame_to_packet(self, frame):
        assert isinstance(frame, bytearray)
        packet = bytearray()
        escape_found = False
        for byte in frame:
            if byte == self.FRAGMENT_ESCAPE_CODE:
                escape_found = True
                continue
            elif escape_found:
                if byte == 0x00:
                    packet.append(self.FRAGMENT_START_CODE)
                elif byte == 0x02:
                    packet.append(self.FRAGMENT_END_CODE)
                elif byte == 0x03:
                    packet.append(self.FRAGMENT_ESCAPE_CODE)
                else:
                    raise SABaseException("Invalid escape symbol %d" % byte)
                escape_found = False
            else:
                packet.append(byte)
        return packet

    def send_packet(self, packet):
        self.transport.write(str(self.packet_to_frame(packet)))

    def frame_received(self, data):
        return self.factory.in_packet_callback(self.frame_to_packet(data))

    def dataReceived(self, data):
        for byte in bytearray(data):
            self._byte_received(byte)

    def _byte_received(self, byte):
        if byte == self.FRAGMENT_START_CODE:
            self._buffer = bytearray()  # clean buffer
            return
        elif byte == self.FRAGMENT_END_CODE:
            return self.frame_received(self._buffer)

        self._buffer.append(byte)

        # keep BUFFER_MAX_LEN
        if len(self._buffer) > self.BUFFER_MAX_LEN:
            self._buffer = self._buffer[self.BUFFER_MAX_LEN * -1:]
