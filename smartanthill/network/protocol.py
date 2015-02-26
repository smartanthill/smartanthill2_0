# Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
#
# Redistribution and use of this file in source and compiled
# forms, with or without modification, are permitted
# provided that the following conditions are met:
#     * Redistributions in source form must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in compiled form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the OLogN Technologies AG nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE

from struct import pack

from twisted.internet import protocol, reactor
from twisted.internet.defer import Deferred

from smartanthill.exception import NetworkSATPMessageLost
from smartanthill.network.cdc import CHANNEL_URGENT
from smartanthill.util import calc_crc16


class ControlMessage(object):

    def __init__(self, cdc, source, destination, ttl=1, ack=False, data=None):
        self.cdc = cdc
        self.source = source
        self.destination = destination
        self.ttl = ttl
        self.ack = ack
        self.data = data or []

        assert (0 <= self.cdc <= 255 and 0 <= self.source <= 255 and
                0 <= self.destination <= 255)
        assert self.source != self.destination
        assert 0 <= self.ttl <= 15, "TTL should be between 1-15"
        assert len(self.data) <= 1792

    def get_channel(self):
        return self.cdc >> 6

    def get_dataclassifier(self):
        return self.cdc & 0x3F

    def is_bdcrequest(self):
        return self.get_channel() == 0x2

    def is_bdcresponse(self):
        return self.get_channel() == 0x3

    def __repr__(self):
        return ("ControlMessage: cdc=0x%X, source=%d, destination=%d, ttl=%d, "
                "ack=%s, data=%s" % (self.cdc, self.source, self.destination,
                                     self.ttl, self.ack, self.data))

    def __eq__(self, other):
        for attr in ("cdc", "source", "destination", "ttl", "ack", "data"):
            if getattr(self, attr) != getattr(other, attr):
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


class ControlProtocol(protocol.Protocol):

    @staticmethod
    def rawmessage_to_message(rawmsg):
        rawmsg = [ord(b) for b in rawmsg]
        message = ControlMessage(cdc=rawmsg[0], source=rawmsg[1],
                                 destination=rawmsg[2],
                                 ack=rawmsg[3] & 0x80 > 0,
                                 ttl=(rawmsg[3] & 0x78) >> 3,
                                 data=rawmsg[5:] if rawmsg[4] else None)
        return message

    def message_received(self, message):
        raise NotImplementedError

    def send_message(self, message):
        assert isinstance(message, ControlMessage)
        flagsandlen = 0x80 if message.ack else 0
        flagsandlen |= (message.ttl) << 3
        flagsandlen |= len(message.data) >> 8  # high 3 bits
        rawmessage = pack("BBBBB", message.cdc, message.source,
                          message.destination, flagsandlen,
                          len(message.data) & 0xF)
        if len(message.data):
            rawmessage += pack("B"*len(message.data), *message.data)
        self.transport.write(rawmessage)

    def dataReceived(self, data):
        self.message_received(self.rawmessage_to_message(data))


class TransportProtocol(protocol.Protocol):

    SEGMENT_FLAG_SEG = 0x4
    SEGMENT_FLAG_FIN = 0x2
    SEGMENT_FLAG_ACK = 0x1

    def __init__(self):
        self._inmsgbuffer = {}
        self._outmsgbuffer = {}

    def send_message(self, message):
        """ Converts Control Message to Segments and sends their """

        ack = ord(message[3]) & 0x80
        ttl = (ord(message[3]) & 0x78) >> 3
        assert 0 < ttl <= 15, "TTL should be between 1-15"

        if ack:
            self._outmsgbuffer[message] = dict(
                d=Deferred(), crcs=[],
                callid=reactor.callLater(ttl, self._messagelost_callback,
                                         message))

        for segment in self.message_to_segments(message):
            if ack:
                self._outmsgbuffer[message]["crcs"].append(segment[-2:])
            self.send_segment(segment)

        return (self._outmsgbuffer[message]["d"] if message in
                self._outmsgbuffer else True)

    def send_segment(self, segment):
        self.transport.write(segment)

    def rawmessage_received(self, message):
        raise NotImplementedError

    def dataReceived(self, data):
        assert len(data) >= 6
        flags = ord(data[3]) >> 5

        # docs/specification/network/cdc/urg.html#cdc-urg-0x0a
        if ord(data[0]) == CHANNEL_URGENT.SEGMENT_ACKNOWLEDGMENT.value:
            return self._acknowledge_received(data)
        elif flags & self.SEGMENT_FLAG_ACK:
            self._send_acknowledge(data)

        # if message isn't segmented
        if not flags & self.SEGMENT_FLAG_SEG and flags & self.SEGMENT_FLAG_FIN:
            return self.rawmessage_received(self.segment_to_message(data))

        # else if segmented then need to wait for another segments
        key = data[0:3]
        order = ord(data[4])
        assert order <= 255

        if key not in self._inmsgbuffer:
            self._inmsgbuffer[key] = {}
        if order not in self._inmsgbuffer[key]:
            self._inmsgbuffer[key][order] = data[3:-2]

        message = self._inbufsegments_to_message(key)
        if message:
            self.rawmessage_received(message)

    @staticmethod
    def segment_to_message(segment):
        message = segment[0:3]
        message += pack("BB", (ord(segment[3]) & 0x20) << 2,
                        ord(segment[3]) & 0xF)
        message += segment[4:-2]
        return message

    @staticmethod
    def message_to_segments(message):
        segments = []
        cdc = message[0]
        sarp = message[1:3]
        flags = (TransportProtocol.SEGMENT_FLAG_ACK if ord(message[3]) & 0x80
                 else 0)
        data_len = (ord(message[3]) & 0x7) | ord(message[4])
        data = list(message[5:])
        segmented = data_len > 8

        if segmented:
            mdps = 7  # max data per segment
            flags |= TransportProtocol.SEGMENT_FLAG_SEG
        else:
            mdps = 8

        segorder = 0
        firstIter = True
        while data or firstIter:
            _data_part = data[:mdps]
            del data[0:mdps]
            firstIter = False

            if segmented:
                _data_part = [pack("B", segorder)] + _data_part
                segorder += 1

            if not data:
                flags |= TransportProtocol.SEGMENT_FLAG_FIN

            segment = cdc + sarp
            segment += pack("B", flags << 5 | len(_data_part))
            segment += "".join(_data_part)
            _crc = calc_crc16([ord(b) for b in segment])
            segment += pack("BB", _crc >> 8, _crc & 0xFF)
            segments.append(segment)

        return segments

    def _send_acknowledge(self, segment):
        acksegment = pack("B", CHANNEL_URGENT.SEGMENT_ACKNOWLEDGMENT.value)
        acksegment += segment[2]
        acksegment += segment[1]
        acksegment += pack("B", self.SEGMENT_FLAG_FIN << 5 | 2)
        acksegment += segment[-2:]
        _crc = calc_crc16([ord(b) for b in acksegment])
        acksegment += pack("BB", _crc >> 8, _crc & 0xFF)
        return self.send_segment(acksegment)

    def _acknowledge_received(self, segment):
        if not self._outmsgbuffer:  # pragma: no cover
            return

        assert len(segment) == 8
        ackcrc = segment[4:6]

        for key, value in self._outmsgbuffer.items():
            if ackcrc in value["crcs"]:
                value["crcs"].remove(ackcrc)
                # if empty CRC list then all segments acknowledged
                if not value["crcs"]:
                    value["callid"].cancel()
                    value["d"].callback(True)
                    del self._outmsgbuffer[key]
                return

    def _messagelost_callback(self, message):
        if message not in self._outmsgbuffer:  # pragma: no cover
            return
        self._outmsgbuffer[message]["d"].errback(
            NetworkSATPMessageLost(message))
        del self._outmsgbuffer[message]

    def _inbufsegments_to_message(self, key):
        seg_nums = len(self._inmsgbuffer[key])
        max_index = max(self._inmsgbuffer[key].keys())
        last_segment = self._inmsgbuffer[key][max_index]
        seg_final = (ord(last_segment[0]) >> 5) & self.SEGMENT_FLAG_FIN

        if not seg_final or seg_nums != max_index+1:
            return None

        data_len = 0
        for s in self._inmsgbuffer[key].itervalues():
            data_len += len(s[2:])

        message = key
        message += pack("BB", data_len >> 8, data_len & 0xFF)
        for _order in range(seg_nums):
            message += self._inmsgbuffer[key][_order][2:]

        return message


class RoutingProtocol(protocol.Protocol):

    PACKET_SOP_CODE = "\x01"
    PACKET_HEADER_LEN = 4
    PACKET_MAXDATA_LEN = 8
    PACKET_CRC_LEN = 2
    PACKET_EOF_CODE = "\x17"

    BUFFER_IN_LEN = 16  # The sum of PACKET_* defines length

    def __init__(self):
        self._inbuffer = [0] * self.BUFFER_IN_LEN

    @staticmethod
    def segment_to_packet(segment):
        return (RoutingProtocol.PACKET_SOP_CODE + segment +
                RoutingProtocol.PACKET_EOF_CODE)

    @staticmethod
    def packet_to_segment(packet):
        return packet[1:-1]

    def packet_received(self, packet):
        raise NotImplementedError

    def send_segment(self, segment):
        """ Converts Transport Segment to Routing Packet and sends it """

        return self.send_packet(RoutingProtocol.segment_to_packet(segment))

    def send_packet(self, packet):
        assert (packet[0] == self.PACKET_SOP_CODE and
                packet[-1] == self.PACKET_EOF_CODE)
        self.transport.write(packet)

    def dataReceived(self, data):
        for d in data:
            self._byte_received(d)

    def _byte_received(self, byte):
        self._buffer_push_byte(byte)

        if byte != self.PACKET_EOF_CODE:
            return

        start = self.BUFFER_IN_LEN - self.PACKET_CRC_LEN \
            - self.PACKET_HEADER_LEN - 2
        for i in reversed(range(0, start+1)):
            if self._inbuffer[i] != self.PACKET_SOP_CODE:
                continue

            if self._buffer_contains_packet(i):
                return self.packet_received(
                    "".join(self._inbuffer[i:]))

    def _buffer_push_byte(self, byte):
        del self._inbuffer[0]
        self._inbuffer.append(byte)

    def _buffer_contains_packet(self, sopindex):
        _data_len = ord(self._inbuffer[sopindex+self.PACKET_HEADER_LEN]) & 0xF

        if (_data_len > self.PACKET_MAXDATA_LEN or
                sopindex + self.PACKET_HEADER_LEN + _data_len +
                self.PACKET_CRC_LEN + 2 != self.BUFFER_IN_LEN):
            return False

        _crc = ord(self._inbuffer[self.BUFFER_IN_LEN -
                                  self.PACKET_CRC_LEN-1]) << 8
        _crc |= ord(self._inbuffer[self.BUFFER_IN_LEN-self.PACKET_CRC_LEN])

        _header_and_data = self._inbuffer[
            sopindex+1:
            sopindex+1+self.PACKET_HEADER_LEN+_data_len]
        return _crc == calc_crc16([ord(b) for b in _header_and_data])


class ControlProtocolWrapping(ControlProtocol):

    def __init__(self, climessage_callback):
        self.climesage_callback = climessage_callback

    def message_received(self, message):
        self.climesage_callback(message)


class TransportProtocolWrapping(TransportProtocol):

    def __init__(self, rawmessage_callback):
        TransportProtocol.__init__(self)
        self.rawmessage_callback = rawmessage_callback

    def rawmessage_received(self, message):
        self.rawmessage_callback(message)


class RoutingProtocolWrapping(RoutingProtocol):

    def __init__(self, inpacket_callback):
        RoutingProtocol.__init__(self)
        self.inpacket_callback = inpacket_callback

    def packet_received(self, packet):
        self.inpacket_callback(packet)
