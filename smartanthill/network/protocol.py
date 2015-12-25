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
        if "siot_mesh_at_root_get_next_update" not in line:
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

    PACKET_DIRECTION_CLIENT_TO_COMMSTACK = 38
    PACKET_DIRECTION_COMMSTACK_TO_CLIENT = 37
    PACKET_DIRECTION_HUB_TO_COMMSTACK = 40
    PACKET_DIRECTION_COMMSTACK_TO_HUB = 35
    PACKET_DIRECTION_COMMSTACK_INTERNAL_ERROR = 47

    def send_data(self, direction, address, payload):
        assert isinstance(payload, bytearray)
        packet = bytearray(pack("HHB", len(payload), address, direction))
        packet.extend(payload)
        self.transport.write(str(packet))
        self.factory.log.debug("Sent packet %s" % hexlify(packet))

    def dataReceived(self, data):
        data = bytearray(data)
        self.factory.log.debug("Received data %s" % hexlify(data))
        direction = data[4]
        if direction == self.PACKET_DIRECTION_COMMSTACK_TO_HUB:
            return self.factory.to_hub_callback(data[5:])
        elif direction == self.PACKET_DIRECTION_COMMSTACK_TO_CLIENT:
            return self.factory.to_client_callback(
                unpack("H", data[2:4]), data[5:])
        elif direction == self.PACKET_DIRECTION_COMMSTACK_INTERNAL_ERROR:
            return self.factory.to_client_errback(
                unpack("H", data[2:4]),
                NetworkCommStackServerInternalError())
        raise SABaseException("Invalid direction %d" % direction)


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
