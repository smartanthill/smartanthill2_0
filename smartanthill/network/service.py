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

from twisted.internet import reactor
from twisted.internet.defer import inlineCallbacks, maybeDeferred, returnValue
from twisted.internet.serialport import SerialPort

import smartanthill.network.protocol as sanp
from smartanthill.exception import NetworkRouterConnectFailure
from smartanthill.service import SAMultiService
from smartanthill.util import get_service_named


class ControlService(SAMultiService):

    def __init__(self, name):
        SAMultiService.__init__(self, name)
        self._protocol = sanp.ControlProtocolWrapping(
            self.climessage_protocallback)
        self._litemq = None

    def startService(self):
        self._litemq = get_service_named("litemq")
        self._protocol.makeConnection(self)
        self._litemq.consume("network", "control.in", "transport->control",
                             self.inmessage_mqcallback)
        self._litemq.consume("network", "control.out", "client->control",
                             self.outmessage_mqcallback)
        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            self._litemq.unconsume("network", "control.in")
            self._litemq.unconsume("network", "control.out")

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def write(self, message):
        self._litemq.produce("network", "control->transport", message,
                             dict(binary=True))

    def inmessage_mqcallback(self, message, properties):
        self.log.debug("Received incoming raw message %s" % hexlify(message))
        self._protocol.dataReceived(message)

    def outmessage_mqcallback(self, message, properties):
        self.log.debug("Received outgoing %s and properties=%s" %
                       (message, properties))
        self._protocol.send_message(message)

    def climessage_protocallback(self, message):
        self.log.debug("Received incoming client %s" % message)
        self._litemq.produce("network", "control->client", message)


class TransportService(SAMultiService):

    def __init__(self, name):
        SAMultiService.__init__(self, name)
        self._protocol = sanp.TransportProtocolWrapping(
            self.rawmessage_protocallback)
        self._litemq = None

    def startService(self):
        self._litemq = get_service_named("litemq")
        self._protocol.makeConnection(self)
        self._litemq.consume("network", "transport.in", "routing->transport",
                             self.insegment_mqcallback)
        self._litemq.consume("network", "transport.out", "control->transport",
                             self.outmessage_mqcallback, ack=True)
        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            self._litemq.unconsume("network", "transport.in")
            self._litemq.unconsume("network", "transport.out")

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def rawmessage_protocallback(self, message):
        self.log.debug("Received incoming raw message %s" % hexlify(message))
        self._litemq.produce("network", "transport->control", message,
                             dict(binary=True))

    def write(self, segment):
        self._litemq.produce("network", "transport->routing", segment,
                             dict(binary=True))

    def insegment_mqcallback(self, message, properties):
        self.log.debug("Received incoming segment %s" % hexlify(message))
        self._protocol.dataReceived(message)

    @inlineCallbacks
    def outmessage_mqcallback(self, message, properties):
        self.log.debug("Received outgoing message %s" % hexlify(message))
        ctrlmsg = sanp.ControlProtocol.rawmessage_to_message(message)

        def _on_err(failure):
            self._litemq.produce("network", "transport->err", ctrlmsg)
            failure.raiseException()

        d = maybeDeferred(self._protocol.send_message, message)
        d.addErrback(_on_err)
        result = yield d
        if result and ctrlmsg.ack:
            self._litemq.produce("network", "transport->ack", ctrlmsg)
        returnValue(result)


class RouterService(SAMultiService):

    RECONNECT_DELAY = 1  # in seconds

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._protocol = sanp.RoutingProtocolWrapping(
            self.inpacket_protocallback)
        self._router_device = None
        self._litemq = None
        self._reconnect_nums = 0
        self._reconnect_callid = None

    def startService(self):
        connection = self.options['connection']
        try:
            if connection.get_protocol() == "serial":
                _kwargs = connection.get_options()
                _kwargs[
                    'deviceNameOrPortNumber'] = connection.get_address()
                _kwargs['protocol'] = self._protocol
                _kwargs['reactor'] = reactor
                self._router_device = SerialPort(**_kwargs)
        except OSError as e:
            self.log.error(str(e))
            self.log.error(NetworkRouterConnectFailure(self.options))
            self._reconnect_nums += 1
            self._reconnect_callid = reactor.callLater(
                self._reconnect_nums * self.RECONNECT_DELAY, self.startService)
            return

        self._litemq = get_service_named("litemq")
        self._litemq.consume(
            exchange="network",
            queue="routing.out." + self.name,
            routing_key="transport->routing",
            callback=self.outsegment_mqcallback
        )

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            if self._reconnect_callid:
                self._reconnect_callid.cancel()
            if self._router_device:
                self._router_device.loseConnection()
            if self._litemq:
                self._litemq.unconsume("network", "routing.out." + self.name)

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d

    def inpacket_protocallback(self, packet):
        self.log.debug("Received incoming packet %s" % hexlify(packet))
        self._litemq.produce("network", "routing->transport",
                             sanp.RoutingProtocol.packet_to_segment(packet),
                             dict(binary=True))

    def outsegment_mqcallback(self, message, properties):
        # check destination ID  @TODO
        if ord(message[2]) not in self.options['deviceids']:
            return False
        self.log.debug("Received outgoing segment %s" % hexlify(message))
        self._protocol.send_segment(message)


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

        print self._protocol, self._address, self._options


class NetworkService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._litemq = None

    def startService(self):
        self._litemq = get_service_named("litemq")
        self._litemq.declare_exchange("network")

        ControlService("network.control").setServiceParent(self)
        TransportService("network.transport").setServiceParent(self)

        devices = get_service_named("device").get_devices()
        for devid, devobj in devices.iteritems():
            connectionUri = devobj.options.get("connectionUri", None)
            if not connectionUri:
                continue

            _options = {"connection": ConnectionInfo(connectionUri),
                        "deviceids": [devid]}
            RouterService("network.router.%d" % devid,
                          _options).setServiceParent(self)

        SAMultiService.startService(self)

    def stopService(self):
        def _on_stop(_):
            self._litemq.undeclare_exchange("network")

        d = SAMultiService.stopService(self)
        d.addCallback(_on_stop)
        return d


def makeService(name, options):
    return NetworkService(name, options)
