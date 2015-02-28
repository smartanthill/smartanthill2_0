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

import os
import sys
from subprocess import check_output
from tempfile import NamedTemporaryFile


CURINTERPRETER_PATH = os.path.normpath(sys.executable)
IS_WINDOWS = sys.platform.startswith("win")


def fix_winpython_pathenv():
    """
    Add Python & Python Scripts to the search path on Windows
    """
    import ctypes
    from ctypes.wintypes import HWND, UINT, WPARAM, LPARAM, LPVOID
    try:
        import _winreg as winreg
    except ImportError:
        import winreg

    # took these lines from the native "win_add2path.py"
    pythonpath = os.path.dirname(CURINTERPRETER_PATH)
    scripts = os.path.join(pythonpath, "Scripts")
    if not os.path.isdir(scripts):
        os.makedirs(scripts)

    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, u"Environment") as key:
        try:
            envpath = winreg.QueryValueEx(key, u"PATH")[0]
        except WindowsError:
            envpath = u"%PATH%"

        paths = [envpath]
        for path in (pythonpath, scripts):
            if path and path not in envpath and os.path.isdir(path):
                paths.append(path)

        envpath = os.pathsep.join(paths)
        winreg.SetValueEx(key, u"PATH", 0, winreg.REG_EXPAND_SZ, envpath)
    winreg.ExpandEnvironmentStrings(envpath)

    # notify the system about the changes
    SendMessage = ctypes.windll.user32.SendMessageW
    SendMessage.argtypes = HWND, UINT, WPARAM, LPVOID
    SendMessage.restype = LPARAM
    SendMessage(0xFFFF, 0x1A, 0, u"Environment")
    return True


def exec_python_cmd(args):
    return check_output([CURINTERPRETER_PATH] + args, shell=IS_WINDOWS).strip()


def install_pip():
    try:
        from urllib2 import urlopen
    except ImportError:
        from urllib.request import urlopen

    f = NamedTemporaryFile(delete=False)
    response = urlopen("https://bootstrap.pypa.io/get-pip.py")
    f.write(response.read())
    f.close()

    try:
        print (exec_python_cmd([f.name]))
    finally:
        os.unlink(f.name)


def install_pypi_packages(packages):
    for p in packages:
        print (exec_python_cmd(["-m", "pip", "install", "-U"] + p.split()))


def main():
    steps = [
        ("Fixing Windows %PATH% Environment", fix_winpython_pathenv, []),
        ("Installing Python Package Manager", install_pip, []),
        ("Installing SmartAnthill and dependencies", install_pypi_packages,
         (["-e git://github.com/ivankravets/smartanthill.git"
           "@develop#egg=smartanthill", "--egg scons"],)),
    ]

    if not IS_WINDOWS:
        del steps[0]

    is_error = False
    for s in steps:
        print ("\n==> %s ..." % s[0])
        try:
            s[1](*s[2])
            print ("[SUCCESS]")
        except Exception, e:
            is_error = True
            print (e)
            print ("[FAILURE]")

    if is_error:
        print ("The installation process has been FAILED!\n"
               "Please report about this problem here\n"
               "< https://github.com/ivankravets/smartanthill/issues >")
        return
    else:
        print ("\n ==> Installation process has been "
               "successfully FINISHED! <==\n")

    try:
        print (check_output("smartanthill --help", shell=IS_WINDOWS))
    except:
        try:
            print (exec_python_cmd(["-m", "smartanthill", "--help"]))
        finally:
            print ("\n Please RESTART your Terminal Application and run "
                   "`smartanthill --help` command.")


if __name__ == "__main__":
    sys.exit(main())
