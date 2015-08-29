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

from os.path import dirname, isdir, isfile, join
from xml.etree import ElementTree

from twisted.python.filepath import FilePath
from twisted.python.util import sibpath

from smartanthill.exception import DeviceUnknownTransport


class DeviceTransport(object):

    def __init__(self, manifest_path):
        self.xml = ElementTree.parse(manifest_path).getroot()
        self._source_dir = dirname(manifest_path)

    def get_source_dir(self):
        return self._source_dir

    def get_id(self):
        return self.xml.get("id")

    def get_name(self):
        return self.xml.get("name")

    def get_description(self):
        return self.xml.find("description").text

    def get_peripheral(self):
        return self._get_items_by_path(
            "./configuration/peripheral",
            ("type", "name", "title")
        )

    def get_options(self):
        options = self._get_items_by_path(
            "./configuration/options",
            ("type", "name", "title", "min", "max", "default")
        )

        # cast attributes to field's type
        for item in (options or []):  # pylint: disable=C0325
            for attr in ("min", "max", "default"):
                if item[attr] is None:
                    continue
                item[attr] = self._cast_to_type(item['type'], item[attr])
            if "_values" in item:
                for _v in item['_values']:
                    _v['value'] = self._cast_to_type(item['type'], _v['value'])
        return options

    def _get_items_by_path(self, path, attrs):
        elements = self.xml.find(path)
        if elements is None:
            return []
        items = []
        for element in elements:
            data = {}

            for attr in attrs:
                data[attr] = element.get(attr, None)

            _values = element.find("./values")
            if _values is not None:
                data['_values'] = []
                for _v in _values:
                    data['_values'].append(
                        {"value": _v.get("value"), "title": _v.get("title")})

            items.append(data)
        return items

    @staticmethod
    def _cast_to_type(type_, value):
        if "int" in type_:
            value = int(value)
        elif "float" in type_:
            value = float(value)
        return value


class DeviceBus(object):

    def __init__(self, transport, id_, name, peripheral=None, options=None):
        assert isinstance(transport, DeviceTransport)
        self.transport = transport

        self._id = id_
        self._name = name
        self._peripheral = peripheral
        self._options = options

    def get_id(self):
        return self._id

    def get_name(self):
        return self._name

    def get_peripheral(self):
        peripheral = self.transport.get_peripheral()
        if not peripheral:
            return []
        for item in peripheral:
            if self._peripheral and item['name'] in self._peripheral:
                item['value'] = self._peripheral[item['name']]
        return peripheral

    def get_options(self):
        options = self.transport.get_options()
        if not options:
            return []
        for item in options:
            if self._options and item['name'] in self._options:
                item['value'] = self._options[item['name']]
        return options


def get_transports(extra_dir=None):
    transports = []
    transports_dirs = [
        sibpath(
            __file__,
            join("..", "cc", "embedded", "firmware", "src", "transports"))
    ]

    if extra_dir:
        transports_dirs.append(extra_dir)

    for transports_dir in transports_dirs:
        if not isdir(transports_dir):
            continue
        for item in FilePath(transports_dir).listdir():
            manifest = join(transports_dir, item, "manifest.xml")
            if isfile(manifest):
                transports.append(DeviceTransport(manifest))
    return sorted(transports, key=lambda item: item.get_id())


def buses_to_objects(buses_raw, transports_extra_dir=None):

    def _get_transport(id_):
        for item in get_transports(transports_extra_dir):
            if item.get_id() == id_:
                return item
        raise DeviceUnknownTransport(id_)

    buses = []
    for id_, item in enumerate(buses_raw):
        buses.append(DeviceBus(
            _get_transport(item['transportId']),
            id_,
            item['name'],
            item.get("peripheral", None),
            item.get("options", None)
        ))
    return buses
