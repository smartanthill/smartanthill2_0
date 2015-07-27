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

from __future__ import absolute_import

import collections
import functools
import json
from os import getenv
from os.path import isfile, join

from platformio.util import exec_command, get_systype
from twisted.internet import reactor


class memoized(object):
    '''
    Decorator. Caches a function's return value each time it is called.
    If called later with the same arguments, the cached value is returned
    (not reevaluated).
    https://wiki.python.org/moin/PythonDecoratorLibrary#Memoize
    '''

    def __init__(self, func):
        self.func = func
        self.cache = {}

    def __call__(self, *args):
        if not isinstance(args, collections.Hashable):
            # uncacheable. a list, for instance.
            # better to not cache than blow up.
            return self.func(*args)
        if args in self.cache:
            return self.cache[args]
        else:
            value = self.func(*args)
            self.cache[args] = value
            return value

    def __repr__(self):
        '''Return the function's docstring.'''
        return self.func.__doc__

    def __get__(self, obj, objtype):
        '''Support instance methods.'''
        return functools.partial(self.__call__, obj)


def singleton(cls):
    """ From PEP-318 http://www.python.org/dev/peps/pep-0318/#examples """
    _instances = {}

    def get_instance(*args, **kwargs):
        if cls not in _instances:
            _instances[cls] = cls(*args, **kwargs)
        return _instances[cls]
    return get_instance


def get_service_named(name):
    """ Returns SmartAnthill Service's instance by specified name  """
    from smartanthill.service import SmartAnthillService
    if name == "sas":
        return SmartAnthillService()
    else:
        return SmartAnthillService().getServiceNamed(name)


def load_config(path):
    with open(path) as fp:
        return json.load(fp)


def merge_nested_dicts(d1, d2):
    for k1, v1 in d1.items():
        if k1 not in d2:
            d2[k1] = v1
        elif isinstance(v1, dict):
            merge_nested_dicts(v1, d2[k1])
    return d2


def where_is_program(program):
    # try OS's built-in commands
    try:
        result = exec_command(
            ["where" if "windows" in get_systype() else "which", program])
        if result['returncode'] == 0 and isfile(result['out'].strip()):
            return result['out'].strip()
    except OSError:
        pass

    # look up in $PATH
    for bin_dir in getenv("PATH", "").split(";"):
        if isfile(join(bin_dir, program)):
            return join(bin_dir, program)
        elif isfile(join(bin_dir, "%s.exe" % program)):
            return join(bin_dir, "%s.exe" % program)

    return program


def fire_defer(d, *args):
    if len(args) == 0:
        args = (None,)
    reactor.callLater(0, d.callback, *args)
    return d


def calc_crc16(dataset):
    crc_table = [
        0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
        0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
    ]

    crc = 0
    tbl_idx = 0

    for data in dataset:
        tbl_idx = crc ^ (data >> (0 * 4))
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4)
        tbl_idx = crc ^ (data >> (1 * 4))
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4)

    return crc & 0xffff
