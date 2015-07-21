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
.. |SASys| replace:: *SmartAnthill System*

.. _configuration:

Configuration
=============

|SA| uses `JSON <http://en.wikipedia.org/wiki/JSON>`_ human-readable format for
data serialization. This syntax is easy for using and reading.

The |SA| *Configuration Parser* gathers data in the next order (steps):

1. Loads predefined :ref:`baseconf` options.
2. Loads options from :ref:`wsdir`.
3. Loads :ref:`consoleopts`.

.. note::
    The *Configuration Parser* redefines options step by step (from #1 to #3).
    The :ref:`consoleopts` step has the highest priority.

.. _baseconf:

Base Configuration
------------------

The *Base Configuration* is predefined in |SASys|.
See `config_base.json <https://github.com/smartanthill/smartanthill2_0/blob/develop/smartanthill/config_base.json>`_.


.. _wsdir:

Workspace Directory
-------------------

|SA| uses ``--workspacedir`` for:

* finding user's specific start-up configuration options. They must be located
  in the ``smartanthill.json`` file. (Check the list of the available options
  `here <https://github.com/smartanthill/smartanthill2_0/blob/develop/smartanthill/config_base.json>`_)
* finding the *Plugins* for |SASys| (should be located in ``plugins`` directory,
  see `examples <https://github.com/smartanthill/smartanthill2_0-embedded/tree/develop/firmware/src/plugins>`_)
* storing the settings about embedded boards/MCUs
* storing the another working data.

.. warning::
    The *Workspace Directory* must have `Written Permission
    <http://en.wikipedia.org/wiki/File_system_permissions>`_

.. _consoleopts:

Console Options
---------------

The simple options that are defined in :ref:`baseconf` can be redefined through
console options for |SA| *Application*.

The whole list of usage options for |SA| are accessible via:

.. code-block:: bash

    smartanthill --help
