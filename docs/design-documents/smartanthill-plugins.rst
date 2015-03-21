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
    DAMAGE SUCH DAMAGE

.. _saplugin:

SmartAnthill Plugins
====================

:Version:   v0.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill Devices use SmartAnthill Plugins to communicate with specific devices.

SmartAnthill Plugins are generally written in C programming language.

Each SmartAnthill Plugin is represented by it's Plugin Handler, and Plugin Manifest.


.. contents::


SmartAnthill Plugin Handler
---------------------------

Each SmartAnthill Plugin has Plugin Handler, usually implemented as two C functions which have the following prototypes:

**plugin_handler_init(const void\* plugin_config, void\* plugin_state )**

**plugin_handler(const void\* plugin_config, void\* plugin_state, const void\* cmd, uint16 cmd_size, MEMORY_HANDLE reply, WaitingFor\* waiting_for)**

See details on MEMORY_HANDLE in 'Plugin API' section below.

SmartAnthill Plugin Manifest
----------------------------

Each SmartAnthill Plugin has Plugin Manifest, which describes input and output of the plugin.

Plugin Manifest is an XML file, with structure which looks as follows:

.. code-block:: xml

  <smartanthill.plugin>
    <command>
      <field name="abc" type="encoded-int<max=2>" />
    </command>
    <reply>
      <field name="xyz" type="encoded-int<max=2>" min="0" max="255">
        <meaning type="float">
          <linear-conversion input-point0="0" output-point0="20.0"
                             input-point1="100" output-point1="40.0">
        </meaning>
      </field>
    </reply>
  </smartanthill.plugin>

Currently supported <field> types are:

  * "encoded-int<max=n>" (using Encoded-Signed-Int<max=> encoding as specified in :ref:`saprotostack` document).
  * "encoded-uint<max=n>" (using Encoded-Unsigned-Int<max=> encoding as specified in :ref:`saprotostack` document).
  * additional data types will be added as needed

<meaning> tag
^^^^^^^^^^^^^

<meaning> tag specifies that while field has type such as integer, it's meaning for the programmer and end-user is different, and can be, for example, a float. This often arises when plugin, for example, measures temperature in range between 35 and 40 celsius as an integer from 0 to 255. <meaning> tag in Plugin Manifest allows developer to write something along the lines of:

**if(TemperatureSensor.Temperature > 38.9) {...}**

instead of

**if(TemperatureSensor.Temperature > 200) {...}**

which would be necessary without <meaning> tag.

To enable much more intuitive first form, an appropriate fragment of Plugin Manifest should be written as

.. code-block:: xml

  ...
    <field name="Temperature" type="encoded-int<max=1>">
      <meaning type="float">
        <linear-conversion input-point0="0" output-point0="35.0"
                           input-point1="255" output-point1="40.0">
      </meaning>
  ...

or as

.. code-block:: xml

  ...
    <field name="Temperature" type="encoded-int<max=1>" min="0" max="99">
      <meaning type="float">
        <linear-conversion a="0.0196" b="35.">
      </meaning>
  ...

where *meaning* is calculated as **meaning=a\*field+b**.

Currently supported <meaning> types are "float" and "int". If <meaning> type is 'int', then all the relevant calculations are performed as floats, and then rounded to the nearest integer.

Each <meaning> tag MUST specify conversion. Currently supported conversions are: <linear-conversion> and <piecewise-linear-conversion> [TODO].

<meaning> tags can be used both for <command> fields and for <reply> fields.

SmartAnthill Plugin Handler as a State Machine
----------------------------------------------

Ideally, SmartAnthill Plugin Handler SHOULD be implemented as state machines, for example:

.. code-block:: c

    struct MyPluginConfig { //constant structure filled with a configuration
                          //  for specific 'ant body part'
    byte request_pin_number;//pin to request sensor read
    byte ack_pin_number;//pin to wait for to see when sensor has provided the data
    byte reply_pin_numbers[4];//pins to read when ack_pin_number shows that the data is ready
    };

    struct MyPluginState {
    byte state; //'0' means 'initial state', '1' means 'requested sensor to perform read'
    };

    byte my_plugin_handler_init(const void* plugin_config,void* plugin_state) {
    //perform sensor initialization if necessary
    MyPluginState* ps = (MyPluginState*)plugin_state;
    ps->state = 0;
    }

    //TODO: reinit? (via deinit, or directly, or implicitly)

    byte my_plugin_handler(const void* plugin_config, void* plugin_state,
      const void* cmd, uint16 cmd_size, MEMORY_HANDLE reply, WaitingFor* waiting_for) {
    const MyPluginConfig* pc = (MyPluginConfig*) plugin_config;
    MyPluginState* ps = (MyPluginState*)plugin_state;
    if(ps->state == 0) {
      //request sensor to perform read, using pc->request_pin_number
      ps->state = 1;
      //let's assume that sensor will set signal on pin#3 to 1 when the data is ready

      //filling in pins_to_wait to indicate we're waiting for pin #3, and value =1 for it:
      byte apn = pc->ack_pin_number;

      //splitting apn into byte number 'idx' and bit number 'shift'
      byte idx = apn >> 3;
      byte shift = apn & 0x7;
      waiting_for->pins_to_wait[idx] |= (1<<shift);
      waiting_for->pins_values_to_wait[idx] |= (1<<shift);

      return WAITING_FOR;
    }
    else {
      //read pin# pc->ack_pin_number just in case
      if(ack_pin != 1) {
        byte apn = pc->ack_pin_number;
        byte idx = apn >> 3;
        byte shift = apn & 0x7;
        waiting_for->pins_to_wait[idx] |= (1<<shift);
        waiting_for->pins_values_to_wait[idx] |= (1<<shift);
        return WAITING_FOR;
      }
      //read data from sensor using pc->reply_pin_numbers[],
      //  and fill in "reply buffer" with data using reply_append(reply,sz)
      //  Note that the pointer returned by reply_append() may change between different
      //    calls to my_plugin_handler() and therefore MUST NOT be stored
      //    within plugin_state
      return 0;
    }

Such an approach allows SmartAnthill implementation (such as Zepto VM) to perform proper pausing (with ability for SmartAnthill Client to interrupt processing by sending a new command while it didn't receive an answer to the previous one), when long waits are needed. It also enables parallel processing of the plugins (see PARALLEL instruction of Zepto VM in :ref:`sazeptovm` document for details).

However, for some plugins (simple ones without waiting at all, or if we're too lazy to write proper state machine), we MAY use 'dummy state machine', with *MyPluginState* being NULL and unused, and **plugin_handler()** not taking into account any states at all.


Plugin API
----------

SmartAnthill implementation MUST provide the following APIs to be used by plugins.

Data Types
^^^^^^^^^^

MEMORY_HANDLE
'''''''''''''

MEMORY_HANDLE is an encapsulation of memory block, which allows plugin to call **reply_append()** (see below). MEMORY_HANDLE is normally obtained as a parameter from plugin_handler() call.

**Caution:** Plugins MUST treat MEMORY_HANDLE as completely opaque and MUST NOT try to use it to access reply buffer directly; doing so may easily result in memory corruption when running certain Zepto VM programs (for example, when PARALLEL instruction is used).

For an information on possible implementations of MEMORY_HANDLE, see :ref:`sazeptoos` document.

TODO: WaitingFor

Functions
^^^^^^^^^

reply_append()
''''''''''''''

**void\* reply_append(MEMORY_HANDLE handle, uint16 sz);**

reply_append() allocates 'sz' bytes within "reply buffer" specified by handle and returns a pointer to this allocated buffer. This buffer can be then filled with plugin's reply.

**Caution:** note that the pointer returned by reply_append() is temporary and may change between different calls to the same plugin, i.e. this pointer (or derivatives) MUST NOT be stored as a part of the plugin state; storing offsets is fine.

TODO: describe error conditions (such as lack of space in buffer)

TODO: parse/compose

