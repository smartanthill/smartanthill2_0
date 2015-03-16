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

# pylint: disable=W0212

from inspect import getargspec

from twisted.internet.defer import Deferred
from twisted.python.failure import Failure
from twisted.test import proto_helpers
from twisted.trial.unittest import TestCase

import smartanthill.network.protocol as p
from smartanthill.exception import NetworkSATPMessageLost
from smartanthill.network.cdc import CHANNEL_BDCREQUEST

CDC_BDC_LIST_OPERATIONS = CHANNEL_BDCREQUEST.LIST_OPERATIONS.value


class NetworkProtocolCase(TestCase):

    def test_controlmessage(self):
        # check arguments
        self.assertEqual(getargspec(p.ControlMessage.__init__).args,
                         ["self", "cdc", "source", "destination", "ttl",
                          "ack", "data"])
        cm1 = p.ControlMessage(CHANNEL_BDCREQUEST.LIST_OPERATIONS.value,
                               0, 255)
        cm2 = p.ControlMessage(CHANNEL_BDCREQUEST.LIST_OPERATIONS.value,
                               0, 255)
        cm3 = p.ControlMessage(CHANNEL_BDCREQUEST.LIST_OPERATIONS.value,
                               1, 255)
        self.assertEqual(cm1.get_channel(), 0x02)
        self.assertEqual(cm1.get_dataclassifier(), 0x09)
        self.assertTrue(cm1.is_bdcrequest())
        self.assertFalse(cm1.is_bdcresponse())
        # compare by cm.__eq__not by reference
        self.assertEqual(cm1, cm2)
        self.assertNotEqual(cm1, cm3)

    def test_controlprotocol(self):
        cm = rawmsg = None
        msgs = (
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255),
             "\x89\x00\xff\x08\x00"),
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, 13, True,
                              range(0, 10)),
             "\x89\x00\xff\xe8\x0a\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09")
        )

        def _message_received(message):
            self.assertEqual(message, cm)

        cp = p.ControlProtocolWrapping(_message_received)
        cp.makeConnection(proto_helpers.StringTransport())

        for (cm, rawmsg) in msgs:
            cp.transport.clear()
            cp.send_message(cm)
            self.assertEqual(cp.transport.value(), rawmsg)
            cp.dataReceived(rawmsg)

    def test_transportprotocol_sending_nonack(self):
        msgs = (
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255),
             "\x89\x00\xff\x40\x5c\x6a"),
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, 13, False,
                              range(0, 10)),
             "\x89\x00\xff\x88\x00\x00\x01\x02\x03\x04\x05\x06\x51\x97"
             "\x89\x00\xff\xc4\x01\x07\x08\x09\x2f\x5a")
        )
        cp, tp = p.ControlProtocol(), p.TransportProtocol()
        cp.makeConnection(proto_helpers.StringTransport())
        tp.makeConnection(proto_helpers.StringTransport())

        for (cm, segments) in msgs:
            cp.transport.clear()
            tp.transport.clear()
            cp.send_message(cm)
            tp.send_message(cp.transport.value())
            self.assertEqual(tp._outmsgbuffer, {})
            self.assertEqual(tp.transport.value(), segments)

    def test_transportprotocol_sending_ack(self):
        msgs = (
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, ack=True),
             "\x89\x00\xff\x60\x84\x6b",
             ("\x0A\xff\x00\x42\x84\x6b\x45\x96",)),
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, ack=True,
                              data=range(0, 10)),
             "\x89\x00\xff\xa8\x00\x00\x01\x02\x03\x04\x05\x06\x90\x0e"
             "\x89\x00\xff\xe4\x01\x07\x08\x09\xe8\xdb",
             ("\x0A\xff\x00\x42\x90\x0e\x6e\x59",
              "\x0A\xff\x00\x42\xe8\xdb\xf1\xba"))
        )
        cp, tp = p.ControlProtocol(), p.TransportProtocol()
        cp.makeConnection(proto_helpers.StringTransport())
        tp.makeConnection(proto_helpers.StringTransport())

        for (cm, segments, acksegments) in msgs:
            cp.transport.clear()
            tp.transport.clear()
            cp.send_message(cm)
            rawmsg = cp.transport.value()
            result = tp.send_message(rawmsg)
            self.assertIsInstance(result, Deferred)
            self.assertIn(rawmsg, tp._outmsgbuffer)
            self.assertEqual(tp.transport.value(), segments)
            for acksegment in acksegments:
                tp.dataReceived(acksegment)
            self.assertEqual(tp._outmsgbuffer, {})

    def test_transportprotocol_sending_acklost(self):
        cp, tp = p.ControlProtocol(), p.TransportProtocol()
        cp.makeConnection(proto_helpers.StringTransport())
        tp.makeConnection(proto_helpers.StringTransport())
        cp.send_message(p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255,
                                         ttl=1, ack=True))
        d = tp.send_message(cp.transport.value())
        self.assertIsInstance(d, Deferred)

        def _msglost_errback(result):
            self.assertIsInstance(result, Failure)
            self.assertTrue(result.check(NetworkSATPMessageLost))

        d.addBoth(_msglost_errback)
        return d

    def test_transportprotocol_receiving(self):
        msg = None
        msgs = (
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, ack=True),
             ("\x89\x00\xff\x60\x84\x6b",),
             "\x0a\xff\x00\x42\x84\x6b\x45\x96"),
            (p.ControlMessage(CDC_BDC_LIST_OPERATIONS, 0, 255, ack=True,
                              data=range(0, 10)),
             ("\x89\x00\xff\xa8\x00\x00\x01\x02\x03\x04\x05\x06\x90\x0e",
              "\x89\x00\xff\xe4\x01\x07\x08\x09\xe8\xdb"),
             ("\x0a\xff\x00\x42\x90\x0e\x6e\x59\x0a\xff"
              "\x00\x42\xe8\xdb\xf1\xba"))
        )

        def _rawmessage_received(message):
            _msg = p.ControlProtocol.rawmessage_to_message(message)
            _msg.ttl = msg.ttl
            _msg.ack = msg.ack
            self.assertEqual(msg, _msg)

        tp = p.TransportProtocolWrapping(_rawmessage_received)
        tp.makeConnection(proto_helpers.StringTransport())

        for (msg, segments, acksegment) in msgs:
            tp.transport.clear()
            for segment in segments:
                tp.dataReceived(segment)
            self.assertEqual(tp.transport.value(), acksegment)

    def test_routingprotocol(self):
        segment = "\x0a\x00\x80\x42\x80\xa5\xc5\x28"
        packet = "\x01\x0a\x00\x80\x42\x80\xa5\xc5\x28\x17"

        self.assertEqual(segment, p.RoutingProtocol.packet_to_segment(packet))

        def _packet_received(_packet):
            self.assertEqual(_packet, packet)

        rp = p.RoutingProtocolWrapping(_packet_received)
        rp.makeConnection(proto_helpers.StringTransport())
        rp.send_segment(segment)
        self.assertEqual(rp.transport.value(), packet)
        rp.dataReceived("\x01any-flow\x17"+packet+"or-broken-data")
