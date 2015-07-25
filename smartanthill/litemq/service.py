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

from twisted.internet.defer import Deferred

from smartanthill.litemq.exchange import ExchangeFactory
from smartanthill.service import SAMultiService


class LiteMQService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._exchanges = {}

    def stopService(self):

        def _on_stop(result):
            if self._exchanges:
                raise Exception("Non-empty exchanges dict: %s"
                                % (self._exchanges,))
            return result

        d = Deferred()
        d.addCallback(_on_stop)
        d.chainDeferred(SAMultiService.stopService(self))
        return d

    def declare_exchange(self, name, type_="direct"):
        if name in self._exchanges:
            self.log.warn("Attempt to declare already declared exchange '%s' "
                          "with type '%s'" % (name, type_))
            return
        self._exchanges[name] = ExchangeFactory().newExchange(name, type_)
        self.log.info("Declared new exchange '%s' with type '%s'" % (
            name, type_))

    def undeclare_exchange(self, name):
        if name not in self._exchanges:
            return
        del self._exchanges[name]
        self.log.info("Undeclared exchange '%s'" % name)

    def produce(self, exchange, routing_key, message, properties=None):
        self._ensure_exchange_declared(exchange)
        self.log.debug(
            "Produce new message '%s' with routing_key '%s' to exchange '%s'" %
            (hexlify(message) if properties and "binary" in properties and
             properties["binary"] else message, routing_key, exchange))
        return self._exchanges[exchange].publish(routing_key, message,
                                                 properties)

    def consume(self, exchange, queue, routing_key, callback, ack=False):
        self._ensure_exchange_declared(exchange)
        self._exchanges[exchange].bind_queue(queue, routing_key, callback, ack)
        self.log.info("Registered consumer with exchange=%s, queue=%s, "
                      "routing_key=%s, ack=%s" % (exchange, queue, routing_key,
                                                  ack))

    def unconsume(self, exchange, queue):
        self._ensure_exchange_declared(exchange)
        self._exchanges[exchange].unbind_queue(queue)
        self.log.info("Unregistered consumer with exchange=%s "
                      "and queue=%s" % (exchange, queue))

    def _ensure_exchange_declared(self, exchange):
        if exchange not in self._exchanges:
            raise Exception("Exchange is not declared: %s" % exchange)


def makeService(name, options):
    return LiteMQService(name, options)
