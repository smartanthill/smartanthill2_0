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

from smartanthill.exception import DeviceNotResponding
from smartanthill.network.protocol import ControlMessage
from smartanthill.util import fire_defer, get_service_named, singleton


@singleton
class ZeroVirtualDevice(object):

    ID = 0x0

    def __init__(self):
        self._litemq = get_service_named("litemq")
        self._litemq.consume("network", "msgqueue", "control->client",
                             self.onresult_mqcallback)
        self._litemq.consume("network", "errqueue", "commstack->err",
                             self.onerr_mqcallback)
        self._resqueue = []

    def request(self, destination, data):
        cm = ControlMessage(self.ID, destination, data)
        self._litemq.produce("network", "client->control", cm)
        return self._defer_result(cm)

    def _defer_result(self, message):
        d = Deferred()
        self._resqueue.append((d, message, 0))
        return d

    def onresult_mqcallback(self, message, properties):
        for item in self._resqueue:
            conds = [
                item[1].source == message.destination,
                item[1].destination == message.source
            ]
            if all(conds):
                self._resqueue.remove(item)
                fire_defer(item[0], message.data)
                return True

    def onerr_mqcallback(self, message, properties):
        for item in self._resqueue:
            if item[1] == message:
                if item[2] == self._litemq.options['resend_max']:
                    self._resqueue.remove(item)
                    item[0].errback(DeviceNotResponding(
                        message.destination, item[2]))
                else:
                    item[2] += 1
                break
