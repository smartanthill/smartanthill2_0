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

from twisted.python.log import addObserver, removeObserver
from twisted.trial.unittest import TestCase

from smartanthill.log import Level, Logger


class LogCase(TestCase):

    def setUp(self):
        addObserver(self._logobserver)
        self.log = Logger("test")
        self._lastlog = None

    def tearDown(self):
        removeObserver(self._logobserver)

    def _logobserver(self, data):
        self._lastlog = data

    def test_emit(self):
        self.log.set_level(Level.DEBUG)

        for l in Level.iterconstants():
            _lname = l.name.lower()

            try:
                getattr(self.log, _lname)("Message", _satraceback=False)
            except SystemExit:
                self.assertEqual(l, Level.FATAL)

            self.assertEqual(self._lastlog['system'], "test" if l == Level.INFO
                             else "test#" + _lname)
            self.assertEqual(self._lastlog['_salevel'], l)

    def test_level(self):
        self.log.set_level(Level.INFO)
        self._lastlog = None

        self.log.debug("Debug message")
        self.assertEqual(self._lastlog, None)

        self.log.info("Info message", some_param=13)
        self.assertEqual(self._lastlog['_salevel'], Level.INFO)
        self.assertEqual(self._lastlog['message'], ('Info message',))
        self.assertEqual(self._lastlog['some_param'], 13)
        self.assertEqual(self._lastlog['isError'], 0)

        self.log.warn("Warn message")
        self.assertEqual(self._lastlog['_salevel'], Level.WARN)
        self.assertEqual(self._lastlog['message'], ('Warn message',))
        self.assertEqual(self._lastlog['isError'], 0)

        self.log.error("Error message")
        self.assertEqual(self._lastlog['_salevel'], Level.ERROR)
        self.assertEqual(self._lastlog['isError'], 1)
