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

import base64
from hashlib import sha1
from os import environ, listdir, makedirs
from os.path import dirname, exists, isdir, isfile, join
from shutil import copytree, rmtree

from smartanthill_zc.api import ZeptoBodyPart
from twisted.internet import utils
from twisted.python.filepath import FilePath
from twisted.python.util import sibpath

from smartanthill import FIRMWARE_VERSION
from smartanthill.cc import srcgen
from smartanthill.log import Logger
from smartanthill.util import where_is_program


class PlatformIOProject(object):

    def __init__(self, project_dir, project_conf, clone_embedded=True):
        self.project_dir = project_dir
        self.project_conf = project_conf

        # clone SmartAnthill Embedded Project
        if clone_embedded:
            self._clone_project(
                sibpath(__file__, join("embedded", "firmware")))

        self._env_name = self._generate_env_name()
        self._defines = []
        self.append_define("SMARTANTHILL", "%02d%02d%02d" % FIRMWARE_VERSION)

    def append_define(self, name, value=None):
        self._defines.append((name, value))

    def get_project_dir(self):
        return self.project_dir

    def get_env_name(self):
        return self._env_name

    def get_env_dir(self):
        return join(self.project_dir, ".pioenvs", self._env_name)

    def get_src_dir(self):
        return join(self.project_dir, "src")

    def src_exists(self, path):
        return exists(join(self.get_src_dir(), path))

    def add_src_content(self, path, content):
        dst_path = join(self.get_src_dir(), path)
        if not isdir(dirname(dst_path)):
            makedirs(dirname(dst_path))
        with open(dst_path, "w") as f:
            f.write(content)

    def run(self, target, options=None):
        self._generate_platformio_ini()

        if target == "upload":
            return PlatformIOUploader(self, options).run()
        else:
            return PlatformIOBuilder(self, options).run()

    def _clone_project(self, dir_path):
        # cache existing project
        if set(listdir(dir_path)) <= set(listdir(self.project_dir)):
            return
        if isdir(self.project_dir):
            rmtree(self.project_dir)
        copytree(dir_path, self.project_dir)

    def _generate_env_name(self):
        data = ["%s_%s" % (k, v) for k, v in self.project_conf.iteritems()]
        shasum = sha1(",".join(sorted(data))).hexdigest()
        return "%s_%s" % (self.project_conf['board'], shasum[:10])

    def _generate_platformio_ini(self):
        project_conf = join(self.project_dir, "platformio.ini")
        options = self.project_conf.copy()
        env_group = "[env:%s]" % self._env_name

        if isfile(project_conf) and env_group in open(project_conf).read():
            return

        if "build_flags" not in options:
            options['build_flags'] = ""

        options['build_flags'] += " " + self._get_srcbuild_flags()

        ini_content = [env_group]
        ini_content.extend(["%s = %s" % (k, v)
                            for k, v in options.iteritems()])

        with open(project_conf, "a") as f:
            f.write("\n%s\n" % "\n".join(ini_content))

    def _get_srcbuild_flags(self):
        flags = []
        for d in self._defines:
            if d[1] is not None:
                flags.append("-D%s=%s" % (d[0], d[1]))
            else:
                flags.append("-D%s" % d[0])
        return " ".join(flags)


class PlatformIOBuilder(object):

    def __init__(self, project, options):
        assert isinstance(project, PlatformIOProject)
        self.project = project
        self.log = Logger("PlatformIO")
        self._options = options

    def run(self):
        d = utils.getProcessOutputAndValue(
            where_is_program("platformio"), args=[
                "--force", "run",
                "-vv",
                "--environment", self.project.get_env_name(),
                "--project-dir", self.project.get_project_dir()
            ] + (["--disable-auto-clean"]
                 if self._options.get("disable-auto-clean", False) else []),
            env=environ
        )
        d.addBoth(self.on_result)
        return d

    def on_result(self, result):
        output = "\n".join(result[0:2])

        # 3-rd item in tuple is "return code" (index=2)
        if result[2] != 0:
            self.log.error(output)
            raise Exception(output)

        self.log.info(output)
        return self.get_firmware_data()

    def get_firmware_data(self):
        env_path = FilePath(self.project.get_env_dir())
        assert env_path.isdir()

        data = {
            "version": ".".join([str(s) for s in FIRMWARE_VERSION]),
            "files": []
        }
        for ext in ["bin", "hex"]:
            for path in env_path.globChildren("firmware*.%s" % ext):
                data['files'].append({
                    "name": path.basename(),
                    "content": base64.b64encode(path.getContent())
                })
        return data


class PlatformIOUploader(object):

    def __init__(self, project, options):
        assert isinstance(project, PlatformIOProject)
        self.project = project
        self.log = Logger("PlatformIO")
        self._options = options

    def run(self):
        d = utils.getProcessOutputAndValue(
            where_is_program("platformio"), args=[
                "--force", "run",
                "--project-dir", self.project.get_project_dir(),
                "--target", "uploadlazy",
                "--disable-auto-clean"
            ] + (["--upload-port", self._options.get("upload_port")]
                 if self._options.get("upload_port") else []),
            env=environ
        )
        d.addBoth(self.on_result)
        return d

    def on_result(self, result):
        output = "\n".join(result[0:2])

        # 3-rd item in tuple is "return code" (index=2)
        if result[2] != 0:
            self.log.error(output)
            raise Exception(output)

        self.log.info(output)
        return {"result": "Please restart device"}


def build_firmware(project_dir, platformio_conf, bodyparts,
                   zepto_conf=None, disable_auto_clean=False):
    assert (set(["platform", "board", "src_filter", "build_flags"]) <=
            set(platformio_conf.keys()))
    assert bodyparts and isinstance(bodyparts[0], ZeptoBodyPart)

    if not zepto_conf:
        zepto_conf = dict(AES_ENCRYPTION_KEY=range(0, 16))

    pio = PlatformIOProject(project_dir, platformio_conf)
    pio.add_src_content(
        "sa_bodypart_list.c", srcgen.BodyPartListC(bodyparts).generate()
    )
    pio.add_src_content(
        "zepto_config.h", srcgen.ZeptoConfigH(zepto_conf).generate()
    )

    # copy user's plugins
    for item in bodyparts:
        if pio.src_exists(join("plugins", item.plugin.get_id())):
            continue
        copytree(
            item.plugin.get_source_dir(),
            join(pio.get_src_dir(), join("plugins", item.plugin.get_id())))

    return pio.run(
        target="build", options={"disable-auto-clean": disable_auto_clean})


def upload_firmware(project_dir, platformio_conf, data):
    assert set(["version", "files", "uploadport"]) <= set(data.keys())

    pio = PlatformIOProject(project_dir, platformio_conf, clone_embedded=False)
    env_dir = pio.get_env_dir()
    if not isdir(env_dir):
        makedirs(env_dir)

    for item in data['files']:
        with open(join(env_dir, item['name']), "wb") as f:
            f.write(base64.b64decode(item['content']))

    return pio.run("upload", {"upload_port": data['uploadport']})
