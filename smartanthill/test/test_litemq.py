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
