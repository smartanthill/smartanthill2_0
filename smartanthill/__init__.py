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

# pylint: disable=W1401

VERSION = (0, 0, "0.dev0")
FIRMWARE_VERSION = (1, 0)

__version__ = ".".join([str(s) for s in VERSION])

__title__ = "smartanthill"
__description__ = "An open IoT system"
__url__ = "http://smartanthill.org"
__docsurl__ = "http://docs.smartanthill.org"

__author__ = "OLogN Technologies AG"
__email__ = "info@o-log-n.com"

__license__ = "GNU GPL v2.0"
__copyright__ = "Copyright (c) 2015, OLogN Technologies AG"

__banner__ = """
      _________________________________________
     /                                        /\\     >< {description} ><
    /        \/             \\\\             __/ /\\    Home:    {home}
   /   ___  _@@    Smart    @@_  ___     /  \\/       Docs:    {docs}
  /   (___)(_)    Anthill    (_)(___)   /__          Issues:  {issues}
 /    //|| ||   {version:^11}   || ||\\\\     /\\         License: {license}
/_______________________________________/ /
\_______________________________________\/           Workspace: #workspace#
 \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \\           Dashboard: #dashboard#
""".format(version=__version__,
           description=__description__,
           home=__url__,
           docs=__docsurl__,
           issues="https://github.com/smartanthill/smartanthill2_0/issues",
           license=__copyright__ + ", " + __license__)
