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

from os.path import basename
from xml.etree import ElementTree


class DevicePlugin(object):

    def __init__(self, manifest_path):
        self.xml = ElementTree.parse(manifest_path).getroot()
        self.source_dir = basename(manifest_path)

    def get_id(self):
        return self.xml.get("id")

    def get_name(self):
        return self.xml.get("name")

    def get_description(self):
        return self.xml.find("description").text

    def get_peripheral(self):
        pins = self.xml.find("./configuration/peripheral")
        if pins is None:
            return None
        result = []
        for element in pins:
            data = {}
            for key in ("type", "name", "title"):
                data[key] = element.get(key, None)
            result.append(data)
        return result

    def get_options(self):
        options = self.xml.find("./configuration/options")
        if options is None:
            return None
        result = []
        for element in options:
            data = {}
            for key in ("type", "name", "title", "default"):
                data[key] = element.get(key, None)
            result.append(data)
        return result
