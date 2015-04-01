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

import sys
import traceback

from twisted.python import log
from twisted.python.constants import FlagConstant, Flags

from smartanthill.configprocessor import ConfigProcessor


class Level(Flags):

    FATAL = FlagConstant()
    ERROR = FlagConstant()
    WARN = FlagConstant()
    INFO = FlagConstant()
    DEBUG = FlagConstant()


class Logger(object):

    def __init__(self, system="-", level=Level.INFO):
        self.system = system
        self.set_level(level)

    def set_level(self, level):
        if not isinstance(level, FlagConstant):
            level = Level.lookupByName(level)
        self._level = level

    def fatal(self, *msg, **kwargs):
        kwargs['_salevel'] = Level.FATAL
        self._emit(*msg, **kwargs)
        sys.exit()

    def error(self, *msg, **kwargs):
        kwargs['_salevel'] = Level.ERROR
        self._emit(*msg, **kwargs)

    def warn(self, *msg, **kwargs):
        kwargs['_salevel'] = Level.WARN
        self._emit(*msg, **kwargs)

    def info(self, *msg, **kwargs):
        kwargs['_salevel'] = Level.INFO
        self._emit(*msg, **kwargs)

    def debug(self, *msg, **kwargs):
        kwargs['_salevel'] = Level.DEBUG
        self._emit(*msg, **kwargs)

    def _emit(self, *msg, **kwargs):
        _system = self.system

        if kwargs['_salevel'].value > self._level.value:
            return
        elif kwargs['_salevel'] != Level.INFO:
            _system = "%s#%s" % (_system, kwargs['_salevel'].name.lower())

        params = dict(system=_system)
        params.update(kwargs)
        if kwargs['_salevel'] == Level.FATAL:
            log.err(*msg, **params)
            if ("_satraceback" not in kwargs or
                    kwargs['_satraceback']):  # pragma: no cover
                traceback.print_stack()
        elif kwargs['_salevel'] == Level.ERROR:
            log.err(*msg, **params)
        else:
            log.msg(*msg, **params)


class Console(object):

    def __init__(self, buffer_size):
        self.buffer_size = buffer_size
        self._messages = []
        log.addObserver(self.on_emit)

    def get_messages(self):
        return self._messages

    def on_emit(self, data):
        level = data['_salevel'].name if "_salevel" in data else None
        self._messages.append(
            (data['message'], data['system'].split("#")[0], level)
        )
