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
\_______________________________________\/           Workspace: #wsdir#
 \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \\           Dashboard: #dashboard#
""".format(version=__version__,
           description=__description__,
           home=__url__,
           docs=__docsurl__,
           issues="https://github.com/smartanthill/smartanthill2_0/issues",
           license=__copyright__ + ", " + __license__)
