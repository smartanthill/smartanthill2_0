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

:Version:   v0.3.0

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill Devices use SmartAnthill Plugins to communicate with specific devices.

SmartAnthill Plugins are generally written in C programming language.

Each SmartAnthill Plugin is represented by it's Plugin Handler, and Plugin Manifest.


.. contents::


SmartAnthill Plugin Handler
---------------------------

Each SmartAnthill Plugin has Plugin Handler, usually implemented as two C functions which have the following prototypes:

**plugin_handler_init(const void\* plugin_config, void\* plugin_state )**

**plugin_handler(const void\* plugin_config, void\* plugin_state, ZEPTO_PARSER* command, REPLY_HANDLE reply, WaitingFor\* waiting_for)**

See details on MEMORY_HANDLE in 'Plugin API' section below.

SmartAnthill Plugin Manifest
----------------------------

Each SmartAnthill Plugin has Plugin Manifest, which describes input and output of the plugin.

Plugin Manifest is an XML file, with structure which looks as follows:

.. code-block:: xml

  <smartanthill.plugin name="MyPlugin" title="My Plugin" version="1.0">

    <description>Short description of plugin's capabilities</description>

    <command>
      <field name="abc" type="encoded-int[max=2]" />
    </command>

    <reply>
      <field name="xyz" type="encoded-int[max=2]" min="0" max="255">
        <meaning type="float">
          <linear-conversion input-point0="0" output-point0="20.0"
                             input-point1="100" output-point1="40.0" />
        </meaning>
      </field>
    </reply>

    <configuration>
      <field type="pin[spi.sclk]" name="pin_spi_sclk" title="SPI SCLK Pin" />
      <field type="pin[spi.mosi]" name="pin_spi_mosi" title="SPI MOSI Pin" />
      <field type="pin[spi.miso]" name="pin_spi_miso" title="SPI MISO Pin" />
      <field type="pin[spi.ss]"   name="pin_spi_ss"   title="SPI SS Pin" />
      <field type="pin[digital]"  name="pin_led1" title="LED 1 Pin" />
      <field type="pin[digital]"  name="pin_led2" title="LED 2 Pin" />
      <field type="uint[2]" name="delay_blink_ms" default="150" title="Delay between blinks, ms" />
      <field type="char[30]" name="welcome_to" default="Welcome to SmartAnthill" />
    </configuration>

  </smartanthill.plugin>

Currently supported <field> types are:

  * ``encoded-int[max=n]`` (using Encoded-Signed-Int[max=] encoding as specified in :ref:`saprotostack` document).
  * ``encoded-uint[max=n]`` (using Encoded-Unsigned-Int[max=] encoding as specified in :ref:`saprotostack` document).
  * additional data types will be added as needed

<meaning> tag
^^^^^^^^^^^^^

``<meaning>`` tag specifies that while field has type such as integer, it's meaning for the programmer and end-user is different, and can be, for example, a float. This often arises when plugin, for example, measures temperature in range between 35 and 40 celsius as an integer from 0 to 255. <meaning> tag in Plugin Manifest allows developer to write something along the lines of:

**if(TemperatureSensor.Temperature > 38.9) {...}**

instead of

**if(TemperatureSensor.Temperature > 200) {...}**

which would be necessary without <meaning> tag.

To enable much more intuitive first form, an appropriate fragment of Plugin Manifest should be written as

.. code-block:: xml

  ...
    <field name="Temperature" type="encoded-int[max=1]">
      <meaning type="float">
        <linear-conversion input-point0="0" output-point0="35.0"
                           input-point1="255" output-point1="40.0">
      </meaning>
  ...

or as

.. code-block:: xml

  ...
    <field name="Temperature" type="encoded-int[max=1]" min="0" max="99">
      <meaning type="float">
        <linear-conversion a="0.0196" b="35.">
      </meaning>
  ...

where *meaning* is calculated as ``meaning=a*field+b``.

Currently supported <meaning> types are "float" and "int". If <meaning> type is 'int', then all the relevant calculations are performed as floats, and then rounded to the nearest integer.

Each ``<meaning>`` tag MUST specify conversion. Currently supported conversions are: ``<linear-conversion>`` and ``<piecewise-linear-conversion>`` [TODO].

``<meaning>`` tags can be used both for ``<command>`` fields and for ``<reply>`` fields.


<configuration> tag
^^^^^^^^^^^^^^^^^^^

``<configuration>`` tag specifies the list of required peripheral, pin numbers,
plugin settings, etc.
This information will be used by :ref:`sacorearchdashser` for configuring
SmartAnthill device.

Allowed field types:

Peripheral
''''''''''

* ``<field type="pin[i2c.*]">`` `Inter-Integrated Circuit <http://en.wikipedia.org/wiki/I²C>`_

    + ``<field type="pin[i2c.sda]">`` - Serial Data Line (SDA)
    + ``<field type="pin[i2c.scl]">`` - Serial Clock Line (SCL)

* ``<field type="pin[spi.*]`` `Serial Peripheral Interface Bus <http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus>`_

    + ``<field type="pin[spi.sclk]">`` - Serial Clock (SCLK, output from master)
    + ``<field type="pin[spi.mosi]">`` - Master Output, Slave Input (MOSI, output from master)
    + ``<field type="pin[spi.miso]">`` - Master Input, Slave Output (MISO, output from slave)
    + ``<field type="pin[spi.ss]">``   - Slave Select (SS, active low, output from master)

* ``<field type="pin[analog]">``
* ``<field type="pin[digital]">``
* ``<field type="pin[pwm]">`` - `Pulse-width modulation <http://en.wikipedia.org/wiki/Pulse-width_modulation>`_

Miscellaneous
'''''''''''''
* ``<field type="int[n]">`` , where ``int[1]`` is equal to ``byte`` type
* ``<field type="uint[n]">``
* ``<field type="char[n]">``

SmartAnthill Plugin Handler as a State Machine
----------------------------------------------

Ideally, SmartAnthill Plugin Handler SHOULD be implemented as state machines, for example:

.. code-block:: c

    struct MyPluginConfig { //constant structure filled with a configuration
                          //  for specific 'ant body part'
    byte bodypart_id;//always present
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
      ZEPTO_PARSER* command, REPLY_HANDLE reply, WaitingFor* waiting_for) {
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
      //  and append response to "reply buffer" with data using zepto_reply_append_byte(reply,data_read)
      return 0;
    }

Such an approach allows SmartAnthill implementation (such as Zepto VM) to perform proper pausing (with ability for SmartAnthill Client to interrupt processing by sending a new command while it didn't receive an answer to the previous one), when long waits are needed. It also enables parallel processing of the plugins (see PARALLEL instruction of Zepto VM in :ref:`sazeptovm` document for details).

However, for some plugins (simple ones without waiting at all, or if we're too lazy to write proper state machine), we MAY use 'dummy state machine', with *MyPluginState* being NULL and unused, and **plugin_handler()** not taking into account any states at all.


Plugin API
----------

SmartAnthill implementation MUST provide the following APIs to be used by plugins.

Zepto Exceptions
^^^^^^^^^^^^^^^^

As SmartAnthill plugins operate in a very restricted environments, SmartAnthill uses a very simplified version of exceptions, which can be implemented completely in C, without any support from compiler or underlying libraries. This is known as Zepto Exceptions and should be used as follows:

Try-catch block:

.. code-block:: c

  if(ZEPTO_TRY()) {
    do_something();
  }

  if(ZEPTO_CATCH()) {
    //exception handling here
    //ZEPTO_CATCH() returns exception code passed in ZEPTO_THROW()
  }

Throwing exception:

.. code-block:: c

  ZEPTO_THROW(exception_code);
  //exception_code has type 'byte'

Intermediate processing (MUST be written after each and ever call to a function-able-to-throw-exception; this is necessary to handle platforms where setjmp/longjmp is not available, but MUST be written regardless of the target platform):

.. code-block:: c

  function_able_to_throw_exception();
  ZEPTO_UNWIND(-1); //returns '-1' in case of exception unwinding

ZEPTO_UNWIND MUST be issued after each function call (except for those function calls which are known not to throw any exceptions) for all valid SmartAnthill Plugins.

Exception Codes
'''''''''''''''

Some Exception Codes are reserved for SmartAnthill. To avoid collisions, user exception codes MUST start from ZEPTO_USER_EXCEPTION.


ZEPTO_ASSERT
''''''''''''

ZEPTO_ASSERT is a way to have trackable assertions in plugin code. ZEPTO_ASSERT(condition) effectively causes ZEPTO_THROW(1) if condition fails. ZEPTO_ASSERT() SHOULD be used instead of usual C assert() calls.

zeptoerr
^^^^^^^^

zeptoerr is a pseudo-stream, somewhat similar to traditional stderr. However, due to hardware limitations, zeptoerr capabilities are very limited, and should be used sparingly.

zeptoerr is intended to be used as follows:

.. code-block:: c

  ZEPTOERR(plugin_config->bodypart_id,"Error: %d",error);

It compiles differently depending on compile-time settings, but generally should have an effect similar to `fprintf(stderr,"Error: %d\n", error);`. To facilitate automated stream decoding in certain modes, the following SHOULD be added to the Plugin Manifest:

.. code-block:: xml

  <zeptoerr>
    <line>Error: %d</line> <!-- text within SHOULD be an EXACT match of the text in ZEPTOERR() call -->
    <line>Error 2: %f</line> <!-- text within SHOULD be an EXACT match of the text in ZEPTOERR() call -->
  </zeptoerr>

ZEPTOERR has very limited support for data types: only %d (and synomym %i), %x, and %f are supported. Formatting modifiers (such as "%02d") are currently not supported at all.

Note that in some cases (for example, if SmartAnthill Device runs out of RAM), SmartAnthill Device MAY truncate zeptoerr pseudo-stream.

For implementation details of zeptoerr, please refer to :ref:`sazeptoos` document.

Data Types
^^^^^^^^^^

REPLY_HANDLE
''''''''''''

REPLY_HANDLE is an encapsulation of request/reply block, which allows plugin to call `zepto_reply_append_*()` (see below). REPLY_HANDLE is normally obtained by plugin as a parameter from plugin_handler() call.

**Caution:** Plugins MUST treat REPLY_HANDLE as completely opaque and MUST NOT try to use it to access reply buffer directly; doing so may easily result in memory corruption when running certain Zepto VM programs (for example, when PARALLEL instruction is used).

For an information on possible implementations of REPLY_HANDLE, see :ref:`sazeptoos` document.

ZEPTO_PARSER structure
''''''''''''''''''''''

ZEPTO_PARSER is an opaque structure (which can be seen as a sort of object where all data should be considered as private). It is used as follows:

.. code-block:: c

  uint16_t sz = zepto_parse_encodeduint2(parser);
  byte b = zepto_parse_byte(parser,sz);

TODO: WaitingFor

TODO: half-float library

Functions
^^^^^^^^^

zepto_reply_append_*()
''''''''''''''''''''''

**void zepto_reply_append_byte(REQUEST_REPLY_HANDLE request_reply, byte data);**

**void zepto_reply_append_encodeduint2(REQUEST_REPLY_HANDLE request_reply, uint16_t data);**

**void zepto_reply_append_encodedint2(REQUEST_REPLY_HANDLE request_reply, int16_t data);**

**void zepto_reply_append_block(REQUEST_REPLY_HANDLE request_reply, void* data, size_t datasz);**

zepto_reply_append_*() appends data to the end of reply buffer, which is specified by request_reply parameter. Any zepto_reply_append_*() call MAY cause re-allocation (which in turn MAY cause moving of any memory block); this is usually not a problem, provided that request_reply is used as a completely opaque handle.

TODO: describe error conditions (such as lack of space in buffer) - longjmp?

ZEPTO_PARSER functions
''''''''''''''''''''''

**byte zepto_parse_byte(ZEPTO_PARSER* parser);**

**uint16_t zepto_parse_encodeduint2(ZEPTO_PARSER* parser);**

**int16_t zepto_parse_encodedint2(ZEPTO_PARSER* parser);**

zepto_parse_*() familty of functions parses data from request (which previously has been composed by zepto_reply_append_*() functions, usually on the other device)

