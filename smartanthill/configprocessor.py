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

import json
import os.path
from copy import deepcopy

from twisted.python.filepath import FilePath
from twisted.python.util import sibpath

from smartanthill.exception import ConfigKeyError
from smartanthill.util import load_config, merge_nested_dicts, singleton


def get_baseconf():
    return load_config(sibpath(__file__, "config_base.json"))


@singleton
class ConfigProcessor(object):

    def __init__(self, wsdir, user_options):
        self.wsconfp = FilePath(os.path.join(wsdir, "smartanthill.json"))

        self._data = get_baseconf()
        self._wsdata = {}
        self._process_workspace_conf()
        self._process_user_options(user_options)

    def __str__(self):
        return str(self._data)

    def _process_workspace_conf(self):
        if (not self.wsconfp.exists() or not
                self.wsconfp.isfile()):  # pragma: no cover
            return
        self._wsdata = load_config(self.wsconfp.path)
        self._data = merge_nested_dicts(self._data, deepcopy(self._wsdata))

    def _process_user_options(self, options):
        assert isinstance(options, dict)
        for k, v in options.iteritems():
            _dyndict = v
            for p in reversed(k.split(".")):
                _dyndict = {p: _dyndict}
            self._data = merge_nested_dicts(self._data, _dyndict)

    def _write_wsconf(self):
        with open(self.wsconfp.path, "w") as f:
            json.dump(self._wsdata, f, sort_keys=True, indent=2)

    def __contains__(self, key):
        try:
            self.get(key)
            return True
        except:
            return False

    def get(self, key_path, default=None):
        try:
            value = self._data
            for k in key_path.split("."):
                value = value[k]
            return value
        except KeyError:
            if default is not None:
                return default
            else:
                raise ConfigKeyError(key_path)

    def update(self, key_path, data, write_wsconf=True):
        newdata = data
        for k in reversed(key_path.split(".")):
            newdata = {k: newdata}

        self.load_data(newdata, write_wsconf)

    def load_data(self, data, write_wsconf=True):
        self._data = merge_nested_dicts(self._data, deepcopy(data))
        self._wsdata = merge_nested_dicts(self._wsdata, data)

        if write_wsconf:
            self._write_wsconf()

    def delete(self, key_path, write_wsconf=True):
        if "." in key_path:
            _parts = key_path.split(".")
            _parent = ".".join(_parts[:-1])
            _delkey = _parts[-1]

            # del from current session
            del self.get(_parent)[_delkey]

            # del from workspace
            _tmpwsd = self._wsdata
            for k in _parent.split("."):
                _tmpwsd = _tmpwsd[k]
            del _tmpwsd[_delkey]
        else:
            del self._data[key_path]
            del self._wsdata[key_path]

        if write_wsconf:
            self._write_wsconf()

    def get_data(self):
        return self._data
