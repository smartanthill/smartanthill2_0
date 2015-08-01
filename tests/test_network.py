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

# pylint: disable=W0212

from inspect import getargspec

from twisted.trial.unittest import TestCase

import smartanthill.network.protocol as p


class NetworkProtocolCase(TestCase):

    def test_control_message(self):
        # check arguments
        self.assertEqual(getargspec(p.ControlMessage.__init__).args,
                         ["self", "source", "destination", "data"])
        cm1 = p.ControlMessage(0, 255, bytearray([0x01, 0x02, 0x03]))
        cm2 = p.ControlMessage(0, 255, bytearray([0x01, 0x02, 0x03]))
        cm3 = p.ControlMessage(0, 255, bytearray([0x01, 0x02, 0x05]))

        # compare by cm.__eq__not by reference
        self.assertEqual(cm1, cm2)
        self.assertNotEqual(cm1, cm3)
