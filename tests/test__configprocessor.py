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
