..  Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source (.rst) and compiled
    (.html, .pdf, etc.) forms, with or without modification, are permitted
    provided that the following conditions are met:
        * Redistributions in source form must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in compiled form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the OLogN Technologies AG nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE

.. |SA| replace:: *SmartAnthill*

.. warning::
    SmartAnthill has not been released yet. The steps decribed below will
    install development verion of SmartAnthill. Don't forget to upgrade
    SmartAnthill using the same installation steps from time to time.

Installation
============

|SA| is written in `Python <https://www.python.org/downloads/>`_ and
works on Mac OS X, Linux, Windows OS and *ARM*-based credit-card sized
computers (`Raspberry Pi <http://www.raspberrypi.org>`_,
`BeagleBone <http://beagleboard.org>`_,
`CubieBoard <http://cubieboard.org>`_).

.. contents::

System requirements
-------------------

* **Operating systems:**
    * Mac OS X
    * Linux, +ARM
    * Windows
* `Python 2.6 or Python 2.7 <https://www.python.org/downloads/>`_

All commands below should be executed in
`Command-line <http://en.wikipedia.org/wiki/Command-line_interface>`_
application:

* *Mac OS X / Linux* this is *Terminal* application.
* *Windows* this is
  `Command Prompt <http://en.wikipedia.org/wiki/Command_Prompt>`_ (``cmd.exe``)
  application.

.. note::
    **Linux Users:** Don't forget to install "udev" rules file
    `99-platformio-udev.rules <https://github.com/platformio/platformio/blob/develop/scripts/99-platformio-udev.rules>`_ (an instruction is located in the file).

    **Windows Users:** Please check that you have correctly installed USB driver
    from board manufacturer

Installation Methods
--------------------

Please *choose one of* the following installation methods:

Super-Quick (Mac / Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~

To install or upgrade |SA| paste that at a *Terminal* prompt
(**you MIGHT need** to run ``sudo`` first, just for installation):

.. code-block:: bash

    [sudo] python -c "$(curl -fsSL https://raw.githubusercontent.com/smartanthill/smartanthill2_0/develop/scripts/get-smartanthill.py)"


Installer Script (Mac / Linux / Windows)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To install or upgrade |SA|, download
`get-smartanthill.py <https://raw.githubusercontent.com/smartanthill/smartanthill2_0/develop/scripts/get-smartanthill.py>`_
script. Then run the following (**you MIGHT need** to run ``sudo`` first,
just for installation):

.. code-block:: bash

    # change directory to folder where is located downloaded "get-smartanthill.py"
    cd /path/to/dir/where/is/located/get-smartanthill.py/script

    # run it
    python get-smartanthill.py

On *Windows OS* it may look like:

.. code-block:: bash

    # change directory to folder where is located downloaded "get-smartanthill.py"
    cd C:\path\to\dir\where\is\located\get-smartanthill.py\script

    # run it
    C:\Python27\python.exe get-smartanthill.py

.. warning::
    **Windows Users**: Don't forget to download and install the latest
    `Python for Windows extensions (PyWin32) <http://sourceforge.net/projects/pywin32/files/pywin32/>`_.

Full Guide
----------

1. Check ``python`` version  (only 2.6-2.7 is supported):

.. code-block:: bash

    $ python --version

**Windows Users** only:

    1. `Download Python 2.7 <https://www.python.org/downloads/>`_ and install it.
    2. Download and install the latest
       `Python for Windows extensions (PyWin32)
       <http://sourceforge.net/projects/pywin32/files/pywin32/>`_.
    3. Add to *PATH* system variable ``;C:\Python27;C:\Python27\Scripts;`` and
       reopen *Command Prompt* (``cmd.exe``) application. Please read this
       article `How to set the path and environment variables in Windows
       <http://www.computerhope.com/issues/ch000549.htm>`_.


2. Check a ``pip`` tool for installing and managing *Python* packages:

.. code-block:: bash

    pip search smartanthill

You should see short information about ``smartanthill`` package.

If your computer does not recognize ``pip`` command, try to install it first
using `these instructions <https://pip.pypa.io/en/latest/installing.html>`_.

3. Install a ``smartanthill`` and related packages:

.. code-block:: bash

    pip install smartanthill && pip install --egg scons

For upgrading the ``smartanthill`` to new version please use this command:

.. code-block:: bash

    pip install -U smartanthill
