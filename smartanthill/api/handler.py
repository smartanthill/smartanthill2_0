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

from twisted.python.constants import FlagConstant, Flags


class APIPermission(Flags):

    EXECUTE = FlagConstant()


class APIHandlerBase(object):

    PERMISSION = None
    KEY = None
    REQUIRED_PARAMS = None

    def __init__(self, action, request_key):
        self.action = action
        self.request_key = request_key.lower()

    def match(self):
        return self.action & self.PERMISSION \
            and self.request_key == self.KEY.lower()

    def check_params(self, params):
        if not self.REQUIRED_PARAMS:
            return True
        params = [s if "[" not in s else s[:s.find("[")]+"[]" for s in params]
        return set(self.REQUIRED_PARAMS) <= set(params)

    def handle(self, data):
        raise NotImplementedError
