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

# pylint: disable=W0212,W0613

from twisted.internet.defer import Deferred, DeferredList
from twisted.python.failure import Failure
from twisted.trial.unittest import TestCase

import smartanthill.litemq.exchange as ex
from smartanthill.exception import LiteMQResendFailed


class LiteMQCase(TestCase):

    g_resent_nums = 0

    def test_declare_exchange(self):
        for type_, class_ in {"direct": ex.ExchangeDirect,
                              "fanout": ex.ExchangeFanout}.items():
            self.assertIsInstance(
                ex.ExchangeFactory().newExchange("exchange_name", type_),
                class_
            )

        self.assertRaises(
            AttributeError,
            lambda: ex.ExchangeFactory().newExchange("exchange_name",
                                                     "unknown-type")
        )

    def test_queue_ack_success(self):
        message, properties = "Test message", {"foo": "bar"}

        def _callback(m, p):
            self.assertEqual(m, message)
            self.assertEqual(p, properties)
            return True

        def _resback(result):
            self.assertIsInstance(result, bool)
            self.assertEqual(result, True)

        q = ex.Queue("queue_name", "routing_key", _callback, ack=True)
        d = q.put(message, properties)
        self.assertIsInstance(d, Deferred)
        d.addCallbacks(_resback)
        return d

    def test_queue_ack_fails(self):
        self.g_resent_nums, resend_max = 0, 3

        def _callback(m, p):
            self.g_resent_nums += 1
            # test exception
            if self.g_resent_nums == 1:
                return 1/0
            # test "ack-invalid" that is equl to False
            else:
                return False

        def _errback(result):
            self.assertIsInstance(result, Failure)
            self.assertTrue(result.check(LiteMQResendFailed))
            self.assertEqual(resend_max, self.g_resent_nums)

        q = ex.Queue("queue_name", "routing_key", _callback, ack=True)
        q.RESEND_MAX = resend_max
        q.RESEND_DELAY = 0
        d = q.put("Test message", {"foo": "bar"})
        self.assertIsInstance(d, Deferred)
        d.addBoth(_errback)
        return d

    def test_queue_nonack(self):
        self.g_resent_nums, resend_max = 0, 3

        def _callback(m, p):
            self.g_resent_nums += 1
            return 1/0

        def _errback(result):
            self.assertNotIsInstance(result, Failure)
            self.assertIsInstance(result, bool)
            self.assertEqual(result, False)
            self.assertEqual(self.g_resent_nums, 1)

        q = ex.Queue("queue_name", "routing_key", _callback, ack=False)
        q.RESEND_MAX = resend_max
        q.RESEND_DELAY = 0
        d = q.put("Test message", {"foo": "bar"})
        self.assertIsInstance(d, Deferred)
        d.addBoth(_errback)
        return d

    def test_exchange_direct(self):
        message, properties = "Test message", {"foo": "bar"}

        def _callback(m, p):
            self.assertEqual(m, message)
            self.assertEqual(p, properties)

        myex = ex.ExchangeFactory().newExchange("exchange_name", "direct")
        myex.bind_queue("queue_name", "routing_key", _callback)

        empty_result = myex.publish("invalid_routing_key", message, properties)
        self.assertEqual(empty_result, [])

        result = myex.publish("routing_key", message, properties)
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 1)
        d = result[0]

        def _resback(result):
            self.assertEqual(result, None)
            myex.unbind_queue("queue_name")
            self.assertEqual(len(myex._queues), 0)

        self.assertIsInstance(d, Deferred)
        d.addCallbacks(_resback)
        return d

    def test_exchange_fanout(self):
        self.g_resent_nums = 0
        message, properties = "Test message", {"foo": "bar"}

        def _callback(m, p):
            self.g_resent_nums += 1
            self.assertEqual(m, message)
            self.assertEqual(p, properties)

        myex = ex.ExchangeFactory().newExchange("exchange_name", "fanout")
        myex.bind_queue("queue_name", "routing_key", _callback)

        result = myex.publish("invalid_routing_key", message, properties)
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 1)
        d1 = result[0]

        result = myex.publish("routing_key", message, properties)
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 1)
        d2 = result[0]

        self.assertIsInstance(d1, Deferred)
        self.assertIsInstance(d2, Deferred)

        dl = DeferredList([d1, d2])

        def _resback(result):
            self.assertEqual(result, [(True, None), (True, None)])

        dl.addCallbacks(_resback)
        return dl
