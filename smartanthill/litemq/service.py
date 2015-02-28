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

from binascii import hexlify

from smartanthill.litemq.exchange import ExchangeFactory
from smartanthill.service import SAMultiService


class LiteMQService(SAMultiService):

    def __init__(self, name, options):
        SAMultiService.__init__(self, name, options)
        self._exchanges = {}

    def stopService(self):
        assert self._exchanges == {}
        SAMultiService.stopService(self)

    def declare_exchange(self, name, type_="direct"):
        if name in self._exchanges:
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
        assert exchange in self._exchanges
        self.log.debug(
            "Produce new message '%s' with routing_key '%s' to exchange '%s'" %
            (hexlify(message) if properties and "binary" in properties and
             properties["binary"] else message, routing_key, exchange))
        return self._exchanges[exchange].publish(routing_key, message,
                                                 properties)

    def consume(self, exchange, queue, routing_key, callback, ack=False):
        assert exchange in self._exchanges
        self._exchanges[exchange].bind_queue(queue, routing_key, callback, ack)
        self.log.info("Registered consumer with exchange=%s, queue=%s, "
                      "routing_key=%s, ack=%s" % (exchange, queue, routing_key,
                                                  ack))

    def unconsume(self, exchange, queue):
        assert exchange in self._exchanges
        self._exchanges[exchange].unbind_queue(queue)
        self.log.info("Unregistered consumer with exchange=%s "
                      "and queue=%s" % (exchange, queue))


def makeService(name, options):
    return LiteMQService(name, options)
