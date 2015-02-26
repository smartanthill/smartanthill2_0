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

import os.path

from twisted.trial.unittest import TestCase

from smartanthill.configprocessor import ConfigProcessor
from smartanthill.exception import ConfigKeyError


class ConfigProcessorCase(TestCase):

    def setUp(self):
        self.config = ConfigProcessor(
            os.path.dirname(os.path.realpath(__file__)),
            {"logger.level": "DEBUG"})

    def test__singleton(self):
        self.assertEqual(self.config, ConfigProcessor())

    def test_structure(self):
        self.assertIn("logger", self.config)
        self.assertIn("services", self.config)
        self.assertTrue(self.config.get("logger.level"))

        _service_structure = set(["enabled", "priority", "options"])
        for v in self.config.get("services").values():
            self.assertEqual(set(v.keys()), _service_structure)

    def test_useroptions(self):
        self.assertEqual(self.config.get("logger.level"), "DEBUG")

    def test_invalidkey(self):
        self.assertRaises(ConfigKeyError, self.config.get, "invalid.key")
        self.assertEqual(self.config.get("invalid.key", "default"), "default")
