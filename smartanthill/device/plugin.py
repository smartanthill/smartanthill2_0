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

from os.path import isdir, isfile, join

from smartanthill_zc.api import ZeptoBodyPart, ZeptoPlugin
from twisted.python.filepath import FilePath
from twisted.python.util import sibpath

from smartanthill.exception import DeviceUnknownPlugin


class DevicePlugin(ZeptoPlugin):

    """
        https://github.com/smartanthill/smartanthill-zepto-compiler/blob/
        develop/smartanthill_zc/api.py
    """
    pass


class DeviceBodyPart(ZeptoBodyPart):

    """
        https://github.com/smartanthill/smartanthill-zepto-compiler/blob/
        develop/smartanthill_zc/api.py
    """
    pass


def get_plugins(extra_dir=None):
    plugins = []
    plugins_dirs = [
        sibpath(
            __file__,
            join("..", "cc", "embedded", "firmware", "src", "plugins"))
    ]

    if extra_dir:
        plugins_dirs.append(extra_dir)

    for plugins_dir in plugins_dirs:
        if not isdir(plugins_dir):
            continue
        for item in FilePath(plugins_dir).listdir():
            manifest = join(plugins_dir, item, "manifest.xml")
            if isfile(manifest):
                plugins.append(DevicePlugin(manifest))
    return sorted(plugins, key=lambda item: item.get_id())


def bodyparts_to_objects(bodyparts_raw, plugins_extra_dir=None):

    def _get_plugin(id_):
        for item in get_plugins(plugins_extra_dir):
            if item.get_id() == id_:
                return item
        raise DeviceUnknownPlugin(id_)

    bodyparts = []
    for id_, item in enumerate(bodyparts_raw):
        bodyparts.append(DeviceBodyPart(
            _get_plugin(item['pluginId']),
            id_,
            item['name'],
            item.get("peripheral", None),
            item.get("options", None)
        ))
    return bodyparts
