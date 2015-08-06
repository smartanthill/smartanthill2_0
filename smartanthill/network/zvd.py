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

from twisted.internet.defer import Deferred

from smartanthill.exception import NetworkRequestCancelled
from smartanthill.network.protocol import ControlMessage
from smartanthill.util import fire_defer, get_service_named, singleton


@singleton
class ZeroVirtualDevice(object):

    ID = 0x0

    def __init__(self):
        self._litemq = get_service_named("litemq")
        self._litemq.consume("network", "msgqueue", "commstack->client",
                             self.onresult_mqcallback, ack=True)
        self._litemq.consume("network", "errqueue", "commstack->error",
                             self.onerr_mqcallback)
        self._resqueue = {}

    def request(self, destination, data):
        cm = ControlMessage(self.ID, destination, data)
        self._litemq.produce("network", "client->commstack", cm)

        if cm.destination in self._resqueue:
            self._cancel_request(cm)

        return self._defer_request(cm)

    def _cancel_request(self, message):
        self._resqueue[message.destination][0].errback(
            NetworkRequestCancelled())
        del self._resqueue[message.destination]

    def _defer_request(self, message):
        self._resqueue[message.destination] = (Deferred(), message)
        return self._resqueue[message.destination][0]

    def _my_message(self, message):
        return (message.destination == self.ID and
                message.source in self._resqueue)

    def onresult_mqcallback(self, message, properties):
        if not self._my_message(message):
            return
        fire_defer(self._resqueue[message.source][0], message.data)
        del self._resqueue[message.source]
        return True

    def onerr_mqcallback(self, message, properties):
        cm, reason = message
        if not self._my_message(cm):
            return
        self._resqueue[cm.source][0].errback(reason)
        del self._resqueue[cm.source]
