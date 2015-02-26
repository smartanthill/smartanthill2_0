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

# pylint: disable=R0903

from twisted.internet import reactor
from twisted.internet.defer import Deferred
from twisted.python.failure import Failure
from twisted.python.reflect import namedObject

from smartanthill.exception import LiteMQACKFailed, LiteMQResendFailed
from smartanthill.log import Logger
from smartanthill.util import get_service_named


class ExchangeFactory(object):

    @staticmethod
    def newExchange(name, type_):
        obj_path = "smartanthill.litemq.exchange.Exchange%s" % type_.title()
        obj = namedObject(obj_path)(name)
        assert isinstance(obj, ExchangeBase)
        return obj


class Queue(object):

    RESEND_DELAY = 1  # in seconds
    RESEND_MAX = 10  # tries

    def __init__(self, name, routing_key, callback, ack):
        assert callable(callback)
        self.log = Logger("litemq.queue")
        self.name = name
        self.routing_key = routing_key
        self.callback = callback
        self.ack = ack

        try:
            options = get_service_named("litemq").options
            self.RESEND_DELAY = options['resend_delay']  # pragma: no cover
            self.RESEND_MAX = options['resend_max']  # pragma: no cover
        except AttributeError:
            pass

    def put(self, message, properties):
        d = Deferred()
        self._defer_message(d, 0, message, properties)
        return d

    def _defer_message(self, resdef, resentnums, message, properties):
        d = Deferred()
        d.addCallback(lambda r, m, p: self.callback(m, p),
                      message, properties)
        d.addCallback(self._d_rescallback, resdef)
        d.addErrback(self._d_errback, resdef, resentnums, message, properties)
        reactor.callLater(self.RESEND_DELAY * resentnums, d.callback, None)

    def _d_rescallback(self, result, resdef):
        if not self.ack or (isinstance(result, bool) and result):
            resdef.callback(result)
        else:
            return Failure(LiteMQACKFailed())

    def _d_errback(self, failure, resdef, resentnums, message, properties):
        self.log.warn(failure, resentnums, message, properties)

        resentnums += 1
        if not self.ack:
            resdef.callback(False)
        elif resentnums < self.RESEND_MAX:
            self._defer_message(resdef, resentnums, message, properties)
        else:
            resdef.errback(LiteMQResendFailed())


class ExchangeBase(object):

    def __init__(self, name):
        self.name = name
        self._queues = {}

    def bind_queue(self, name, routing_key, callback, ack=False):
        assert name not in self._queues
        self._queues[name] = Queue(name, routing_key, callback, ack)

    def unbind_queue(self, name):
        if name in self._queues:
            del self._queues[name]

    def publish(self, routing_key, message, properties):
        raise NotImplementedError


class ExchangeDirect(ExchangeBase):

    def publish(self, routing_key, message, properties):
        result = []
        for q in self._queues.itervalues():
            if q.routing_key == routing_key:
                result.append(q.put(message, properties))
        return result


class ExchangeFanout(ExchangeBase):

    def publish(self, routing_key, message, properties):
        result = []
        for q in self._queues.itervalues():
            result.append(q.put(message, properties))
        return result


# class ExchangeTopic(ExchangeBase):

#     def publish(self, routing_key, message, properties):
#         raise NotImplemnetedYet

#     def match(self, routing_key, routing_pattern):
#         raise NotImplementedError
