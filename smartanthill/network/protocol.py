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
from struct import pack

from twisted.internet import protocol


class ControlMessage(object):

    def __init__(self, source, destination, data=None):
        self.source = source
        self.destination = destination
        self.data = data or []

        assert 0 <= self.source <= 255 and 0 <= self.destination <= 255
        assert self.source != self.destination

    def __repr__(self):
        return ("ControlMessage: source=%d, destination=%d, data=%s" % (
            self.source, self.destination, hexlify(self.data)))

    def __eq__(self, other):
        for attr in ("source", "destination", "data"):
            if getattr(self, attr) != getattr(other, attr):
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


class CommStackProtocol(protocol.Protocol):

    PACKET_DIRECTION_CLIENT_TO_COMMSTACK = 38
    PACKET_DIRECTION_COMMSTACK_TO_CLIENT = 37
    PACKET_DIRECTION_DATALINK_TO_COMMSTACK = 40
    PACKET_DIRECTION_COMMSTACK_TO_DATALINK = 35

    def send_data(self, direction, data):
        if not isinstance(data, bytearray):
            data = bytearray(data)
        packet = pack("HB", len(data), direction)
        packet += pack("B" * len(data), *data)
        self.transport.write(packet)

    def dataReceived(self, data):
        direction = ord(data[2])
        if direction == self.PACKET_DIRECTION_COMMSTACK_TO_DATALINK:
            return self.factory.to_datalink_callback(data[3:])
        elif direction == self.PACKET_DIRECTION_COMMSTACK_TO_CLIENT:
            return self.factory.to_client_callback(data[3:])
        assert "Invalid direction %d" % direction


class DataLinkSerialProtocol(protocol.Protocol):

    FRAGMENT_START_CODE = 0x01
    FRAGMENT_ESCAPE_CODE = 0xff
    FRAGMENT_END_CODE = 0x17

    BUFFER_MAX_LEN = 300

    def __init__(self):
        self._buffer = []

    def packet_to_fragment(self, packet):
        fragment = [self.FRAGMENT_START_CODE]
        for byte in packet:
            byte = ord(byte)
            if byte == self.FRAGMENT_START_CODE:
                fragment.extend([self.FRAGMENT_ESCAPE_CODE, 0x00])
            elif byte == self.FRAGMENT_END_CODE:
                fragment.extend([self.FRAGMENT_ESCAPE_CODE, 0x02])
            elif byte == self.FRAGMENT_ESCAPE_CODE:
                fragment.extend([self.FRAGMENT_ESCAPE_CODE, 0x03])
            else:
                fragment.append(byte)
        fragment.append(self.FRAGMENT_END_CODE)
        return "".join([chr(c) for c in fragment])

    def fragment_to_packet(self, fragment):
        packet = []
        escape_found = False
        for byte in fragment:
            byte = ord(byte)
            if byte == self.FRAGMENT_ESCAPE_CODE:
                escape_found = True
                continue
            elif escape_found:
                if byte == 0x00:
                    packet.append(self.FRAGMENT_START_CODE)
                elif byte == 0x02:
                    packet.append(self.FRAGMENT_END_CODE)
                if byte == 0x03:
                    packet.append(self.FRAGMENT_ESCAPE_CODE)
                assert "Invalid escape symbol %d" % byte
            else:
                packet.append(byte)
        return "".join([chr(c) for c in packet])

    def send_packet(self, packet):
        self.transport.write(self.packet_to_fragment(packet))

    def fragment_received(self, data):
        return self.factory.in_packet_callback(self.fragment_to_packet(data))

    def dataReceived(self, data):
        for byte in data:
            self._byte_received(byte)

    def _byte_received(self, byte):
        if ord(byte) == self.FRAGMENT_START_CODE:
            self._buffer = []  # clean buffer
            return
        elif ord(byte) == self.FRAGMENT_END_CODE:
            return self.fragment_received(self._buffer)

        self._buffer.append(byte)

        # keep BUFFER_MAX_LEN
        if len(self._buffer) > self.BUFFER_MAX_LEN:
            self._buffer = self._buffer[self.BUFFER_MAX_LEN * -1:]
