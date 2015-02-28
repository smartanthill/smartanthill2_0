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

from inspect import getmembers, isroutine

from smartanthill.exception import OperArgInvalid


class ArgBase(object):

    def __init__(self):
        self._value = None

    def set_value(self, value):
        self._value = value

    def get_value(self):
        return self._value

    def raise_exception(self, invalid_value):
        members = getmembers(self, lambda a: not isroutine(a))
        attrs = [a for a in members if not a[0].startswith("__") and a[0] !=
                 "_value"]
        raise OperArgInvalid(self.__class__.__name__, attrs, invalid_value)

    def __repr__(self):
        return "%s: value=%s" % (self.__class__.__name__, self._value)


class IntArgBase(ArgBase):

    def __init__(self, min_=None, max_=None, range_=None):
        ArgBase.__init__(self)
        if range_:
            assert len(range_) and (min_ and max_) is None
        else:
            assert range_ is None and (min_ or max_) is not None
        self._min = min_
        self._max = max_
        self._range = range_

    def set_value(self, value):

        try:
            _value = int(value)
            if self._range:
                self._range.index(_value)
            elif self._min is None:
                assert _value <= self._max
            elif self._max is None:
                assert _value >= self._min
            else:
                assert self._min <= _value <= self._max

            ArgBase.set_value(self, _value)
        except:
            self.raise_exception(value)


class IntRangeWithAliasArgBase(IntArgBase):

    def __init__(self, range_, alias):
        IntArgBase.__init__(self, range_=range_)
        self._alias = alias

    def set_value(self, value):
        IntArgBase.set_value(self, self._alias[value] if value in self._alias
                             else value)


class DeviceIDArg(IntArgBase):

    def __init__(self):
        IntArgBase.__init__(self, 1, 255)


class PinArg(IntRangeWithAliasArgBase):

    def __init__(self, allowed, alias=None):
        IntRangeWithAliasArgBase.__init__(self, allowed, alias or [])


class PinLevelArg(IntRangeWithAliasArgBase):

    ALIAS = dict(
        LOW=0,
        HIGH=1
    )

    def __init__(self):
        IntRangeWithAliasArgBase.__init__(self, [0, 1], self.ALIAS)


class PinModeArg(IntRangeWithAliasArgBase):

    def __init__(self, allowed, alias=None):
        IntRangeWithAliasArgBase.__init__(self, allowed, alias or [])


class PinAnalogRefArg(IntRangeWithAliasArgBase):

    def __init__(self, allowed, alias=None):
        IntRangeWithAliasArgBase.__init__(self, allowed, alias or [])


class PinPWMValueArg(IntArgBase):

    def __init__(self):
        IntArgBase.__init__(self, 0, 255)
