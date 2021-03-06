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

import re
from os.path import join
from string import Template


class SourceGenerator(object):

    COPYRIGHT = """
/*******************************************************************************
Copyright (C) 2015 OLogN Technologies AG

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/

/*******************************************************************************
THIS FILE IS AUTOMATICALLY GENERATED BASED ON DESIRED PLUGIN LIST, SETTINGS
*******************************************************************************/
"""

    C_STRUCT_RE = re.compile(
        r"typedef\s+struct\s+\w+\s*"
        "\{\s*([^\}]+)\s*\}\s+(\w+)\s*;", re.M)
    C_STRUCT_MEMBER_RE = re.compile(r"(\w+)\s*\*?\s*(\w+)(\[\d+\])?\s*;", re.M)

    STRIP_COMMENTS_RE = re.compile(
        r"(/\*.*?\*/|\s+//[^\r\n]*$)", re.M | re.S)

    def get_content(self):
        raise NotImplementedError

    def generate(self):
        return "%s\n%s" % (self.COPYRIGHT.strip(), self.get_content())

    def parse_c_structs(self, path):
        content = ""
        with open(path) as f:
            content = f.read()
        content = self.STRIP_COMMENTS_RE.sub("", content)

        members = {}
        for item in self.C_STRUCT_RE.findall(content):
            members[item[1]] = self.C_STRUCT_MEMBER_RE.findall(item[0])
        return members


class ZeptoConfigH(SourceGenerator):

    TPL = Template("""

#if !defined __ZEPTO_CONFIG_H__
#define __ZEPTO_CONFIG_H__

#define DECLARE_AES_ENCRYPTION_KEY \\
const uint8_t AES_ENCRYPTION_KEY[16] ZEPTO_PROG_CONSTANT_LOCATION = \\
{ ${AES_ENCRYPTION_KEY} }; \\

#define DECLARE_DEVICE_ID \\
uint16_t DEVICE_SELF_ID = 1;

#endif // __ZEPTO_CONFIG_H__
""")

    def __init__(self, config):
        self.config = config

    def get_content(self):
        return self.TPL.substitute(
            AES_ENCRYPTION_KEY=", ".join(
                [str(s) for s in self.config['AES_ENCRYPTION_KEY']])
        )


class BodyPartListCPP(SourceGenerator):

    TPL = Template("""
#include <simpleiot/siot_bodypart_list.h>

// include declarations of respective plugins
${plugin_includes}

${plugin_configs}

${plugin_states}

const uint8_t SA_BODYPARTS_MAX ZEPTO_PROG_CONSTANT_LOCATION = ${bodypart_nums};
const bodypart_item bodyparts[${bodypart_nums}] ZEPTO_PROG_CONSTANT_LOCATION =
{
    ${bodypart_items}
};
""")

    def __init__(self, bodyparts):
        self.bodyparts = bodyparts

        self._c_structs = {}
        for bodypart in self.bodyparts:
            self._c_structs[bodypart.get_id()] = self.parse_c_structs(
                join(bodypart.plugin.get_source_dir(),
                     "%s.h" % bodypart.plugin.get_id())
            )

    def get_content(self):
        return self.TPL.substitute(
            bodypart_nums=len(self.bodyparts),
            plugin_includes="\n".join(self._gen_plugin_includes()),
            plugin_configs="\n".join(self._gen_plugin_configs()),
            plugin_states="\n".join(self._gen_plugin_states()),
            bodypart_items=",\n".join(self._gen_bodypart_items()),
        )

    def _gen_plugin_includes(self):
        includes = set()
        for bodypart in self.bodyparts:
            includes.add('#include "plugins/{pid}/{pid}.h"'.format(
                pid=bodypart.plugin.get_id()))
        return list(includes)

    def _gen_plugin_configs(self):
        configs = []
        for bodypart in self.bodyparts:
            data = self._get_plugin_conf_data(bodypart)
            if data['hapi_gpio_vars']:
                configs.extend(data['hapi_gpio_vars'])
            config = (
                "{pid}_plugin_config {pid}_plugin_config_{bpid}"
                .format(bpid=bodypart.get_id(),
                        pid=bodypart.plugin.get_id())
            )
            if data['struct_values']:
                config += "={ %s }" % ", ".join(data['struct_values'])
            configs.append(config + ";")

        return configs

    def _get_plugin_conf_data(self, bodypart):
        data = {
            "hapi_gpio_vars": [],
            "struct_values": []
        }
        c_struct_name = "%s_plugin_config" % bodypart.plugin.get_id()
        bodypart_id = bodypart.get_id()
        c_structs = self._c_structs[bodypart_id]
        assert c_struct_name in c_structs

        for s_item in c_structs[c_struct_name]:
            found_member = False
            for items in (bodypart.get_peripheral(), bodypart.get_options()):
                if not items:
                    continue
                for item in items:
                    if item['name'] != s_item[1]:
                        continue
                    else:
                        found_member = True

                    if s_item[0] == "hapi_gpio_t":
                        _hal_gpio = "hal_gpio_%s_%d" % (
                            item['name'], bodypart_id)
                        _hapi_gpio = "hapi_gpio_%s_%d" % (
                            item['name'], bodypart_id)
                        data['hapi_gpio_vars'].append(
                            "hal_gpio_t %s;" % _hal_gpio
                        )
                        data['hapi_gpio_vars'].append(
                            "hapi_gpio_t %s = {%s, (void*)&%s};" % (
                                _hapi_gpio, item['value'], _hal_gpio)
                        )
                        data['struct_values'].append("&%s" % _hapi_gpio)
                    elif item['type'].startswith("char["):
                        data['struct_values'].append('"%s"' % item['value'])
                    else:
                        data['struct_values'].append("%s" % item['value'])

            if not found_member and "dummy" not in s_item[1]:
                raise Exception("Invalid struct member %s for %s" % (
                    str(s_item), c_struct_name))
        return data

    def _gen_plugin_states(self):
        states = []
        for bodypart in self.bodyparts:
            states.append(
                "{pid}_plugin_state {pid}_plugin_state_{bpid};\n"
                "{pid}_plugin_persistent_state "
                "{pid}_plugin_persistent_state_{bpid};"
                .format(pid=bodypart.plugin.get_id(), bpid=bodypart.get_id())
            )
        return states

    def _gen_bodypart_items(self):
        bpitems = []
        for bodypart in self.bodyparts:
            bpitems.append(
                "{{ {pid}_plugin_handler_init, {pid}_plugin_exec_init, "
                "{pid}_plugin_handler, &{pid}_plugin_config_{bpid}, "
                "&{pid}_plugin_persistent_state_{bpid}, NULL }}"
                .format(pid=bodypart.plugin.get_id(), bpid=bodypart.get_id())
            )
        return bpitems


class BusListCPP(SourceGenerator):

    TPL = Template("""
#include "sa_bus_list.h"
""")

    def __init__(self, buses):
        self.buses = buses

    def get_content(self):
        return self.TPL.substitute()
