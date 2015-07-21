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

Launching
=========

|SA| is based on `Twisted <http://en.wikipedia.org/wiki/Twisted_(software)>`_
and can be launched as *Foreground Process* as well as
`Background Process <http://en.wikipedia.org/wiki/Background_process>`_.

Foreground Process
------------------

The whole list of usage options for |SA| is accessible via:

.. code-block:: bash

    smartanthill --help

Quick launching (the user's home directory ``${HOME}`` will be used as
:ref:`wsdir`):

.. code-block:: bash

    smartanthill

Launching with specific :ref:`wsdir`:


.. code-block:: bash

    smartanthill --workspacedir=/path/to/workspace/directory


Check the :ref:`configuration` page for detailed configuration options.


Background Process
------------------

The launching in the *Background Process* implements through ``twistd`` utility.
The whole list of usage options for ``twistd`` is accessible via
``twistd --help`` command. The final |SA| command looks like:

.. code-block:: bash

    twistd smartanthill

Dashboard (GUI)
---------------

SmartAnthill Dashboard is accessible by default on
`http://localhost:8138 <http://localhost:8138>`_.

However, you can change TCP/IP port later using ``Dashboard > Settings`` page.
