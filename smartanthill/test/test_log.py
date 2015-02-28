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
