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


import re

from smartanthill.exception import WebRouterMatchNotFound


class WebRouter(object):

    def __init__(self, prefix=""):
        self.prefix = prefix
        self._routes = []

    def add(self, path, method="GET"):
        def decorator(callback):
            self._routes.append(Route(self.prefix+path, method, callback))
            return callback
        return decorator

    def match(self, request):
        for route in self._routes:
            if route.match(request):
                return route.invoke()
        raise WebRouterMatchNotFound(request.method, request.path)


class Route(object):

    def __init__(self, path, method, callback):
        self.path = path
        self.method = method if isinstance(method, list) else [method]
        self.callback = callback

        self._request = None
        self._resmatch = None
        self._param_types = {}

        self.repath = self._path_to_re(path)

    def match(self, request):
        self._request = request

        if request.method not in self.method:
            return False

        self._resmatch = self.repath.match(request.path)
        return self._resmatch is not None

    def invoke(self):
        _kwargs = {"request": self._request}
        if self._resmatch:
            for k, v in self._resmatch.groupdict().iteritems():
                if self._param_types[k] == "int":
                    _kwargs[k] = int(v)
                elif self._param_types[k] == "float":
                    _kwargs[k] = float(v)
                else:
                    _kwargs[k] = v
        return self.callback(**_kwargs)

    def _path_to_re(self, path):
        if "<" in path and ">" in path:
            tpl = re.compile(r"(?:\<(?:(int|float|path):)?([a-z_]+)\>)+", re.I)
            path = tpl.sub(self._retpl_callback, path)
        return re.compile(path)

    def _retpl_callback(self, match):
        _type = match.group(1).lower() if match.group(1) else None
        _name = match.group(2)
        self._param_types[_name] = _type
        if _type == "int":
            return r"(?P<%s>\d+)" % _name
        elif _type == "float":
            return r"(?P<%s>[\d\.]+)" % _name
        elif _type == "path":
            return r"(?P<%s>.+)" % _name
        else:
            return r"(?P<%s>[^/]+)" % _name
