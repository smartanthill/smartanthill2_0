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

:Version: v0.4.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill Devices use SmartAnthill Plugins to communicate with specific devices.

SmartAnthill Plugins are generally written in C programming language.

Each SmartAnthill Plugin is represented by it's Plugin Handler, and Plugin Manifest.


.. contents::


SmartAnthill Plugin Handler
---------------------------

Each SmartAnthill Plugin has Plugin Handler, usually implemented as two C functions which have the following prototypes:

**plugin_handler_init(const void\* plugin_config, void\* plugin_state )**

**plugin_handler(const void\* plugin_config, void\* plugin_state, ZEPTO_PARSER* command, REPLY_HANDLE reply, WAITING_FOR\* waiting_for)**

See details on MEMORY_HANDLE in 'Plugin API' section below.

SmartAnthill Plugin Manifest
----------------------------

Each SmartAnthill Plugin has Plugin Manifest, which describes input and output of the plugin.

Plugin Manifest is an XML file, with structure which looks as follows:

.. code-block:: xml

  <smartanthill.plugin id="my" name="My Plugin" version="1.0">

    <description>Short description of plugin's capabilities</description>

    <request>
      <field name="abc" type="encoded-int[max=2]" />
    </request>

    <response>
      <field name="xyz" type="encoded-int[max=2]" min="0" max="255">
        <meaning type="float">
          <linear-conversion input-point0="0" output-point0="20.0"
                             input-point1="100" output-point1="40.0" />
        </meaning>
      </field>
    </response>

    <configuration>
      <peripheral>
        <pin type="spi[sclk]" name="pin_spi_sclk" title="SPI SCLK Pin" />
        <pin type="spi[mosi]" name="pin_spi_mosi" title="SPI MOSI Pin" />
        <pin type="spi[miso]" name="pin_spi_miso" title="SPI MISO Pin" />
        <pin type="spi[ss]"   name="pin_spi_ss"   title="SPI SS Pin" />
        <pin type="digital"   name="pin_led1"     title="LED 1 Pin" />
        <pin type="digital"   name="pin_led2"     title="LED 2 Pin" />
      </peripheral>
      <options>
        <option type="uint[2]" name="delay_blink_ms" default="150" title="Delay between blinks, ms" />
        <option type="char[30]" name="welcome_to" default="Welcome to SmartAnthill" title="Welcome text" />
      </options>
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
plugin options, etc.
This information will be used by :ref:`sacorearchdashser` for configuring
SmartAnthill device.

Allowed field types:

Peripheral
''''''''''

* ``<pin type="i2c[*]">`` `Inter-Integrated Circuit <http://en.wikipedia.org/wiki/I²C>`_

    + ``<pin type="i2c[sda]">`` - Serial Data Line (SDA)
    + ``<pin type="i2c[scl]">`` - Serial Clock Line (SCL)

* ``<pin type="spi[*]`` `Serial Peripheral Interface Bus <http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus>`_

    + ``<pin type="spi[sclk]">`` - Serial Clock (SCLK, output from master)
    + ``<pin type="spi[mosi]">`` - Master Output, Slave Input (MOSI, output from master)
    + ``<pin type="spi[miso]">`` - Master Input, Slave Output (MISO, output from slave)
    + ``<pin type="spi[ss]">``   - Slave Select (SS, active low, output from master)

* ``<pin type="analog">``
* ``<pin type="digital">``
* ``<pin type="pwm">`` - `Pulse-width modulation <http://en.wikipedia.org/wiki/Pulse-width_modulation>`_

Options
'''''''

* ``<option type="int[n]">`` , where ``int[1]`` is equal to ``byte`` type
* ``<option type="uint[n]">``
* ``<option type="char[n]">``

SmartAnthill Plugin Handler Without a State Machine
---------------------------------------------------

Simple SA plugins MAY be written without being a State Machine, for example:

.. code-block:: c

    struct my_plugin_config { //constant structure filled with a configuration
                          //  for specific 'ant body part'
      byte bodypart_id;//always present
      byte request_pin_number;//pin to request sensor read
      byte ack_pin_number;//pin to wait for to see when sensor has provided the data
      byte reply_pin_numbers[4];//pins to read when ack_pin_number shows that the data is ready
    };

    byte my_plugin_handler_init(const void* plugin_config,void* plugin_state) {
      const my_plugin_config* pc = (my_plugin_config*) plugin_config;
      zepto_set_pin(pc->request_pin_number,0);
    }

    //TODO: reinit? (via deinit, or directly, or implicitly)

    byte my_plugin_handler(const void* plugin_config, void* plugin_state,
      ZEPTO_PARSER* command, REPLY_HANDLE reply, WAITING_FOR* waiting_for) {
      const my_plugin_config* pc = (my_plugin_config*) plugin_config;

      //requesting sensor to perform read, using pc->request_pin_number
      zepto_set_pin(pc->request_pin_number,1);

      //waiting for sensor to indicate that data is ready
      zepto_wait_for_pin(pc->ack_pin_number,1);

      uint16_t data_read = zepto_read_from_pins(pc->reply_pin_numbers,4);
      zepto_reply_append_byte(reply,data_read);
      return 0;
    }


SmartAnthill Plugin Handler as a State Machine
----------------------------------------------

Implementation above is not ideal; in fact, it blocks execution at the point of zepto_wait_for_pin() call, which under restrictions of Zepto OS means that nothing else can be processed. Ideally, SmartAnthill Plugin Handler SHOULD be implemented as a state machine; for example, the very same plugin SHOULD be rewritten as follows:

.. code-block:: c

    struct my_plugin_config { //constant structure filled with a configuration
                          //  for specific 'ant body part'
      byte bodypart_id;//always present
      byte request_pin_number;//pin to request sensor read
      byte ack_pin_number;//pin to wait for to see when sensor has provided the data
      byte reply_pin_numbers[4];//pins to read when ack_pin_number shows that the data is ready
    };

    struct my_plugin_state {
      byte state; //'0' means 'initial state', '1' means 'requested sensor to perform read'
    };

    byte my_plugin_handler_init(const void* plugin_config,void* plugin_state) {
      my_plugin_state* ps = (my_plugin_state*)plugin_state;
      const my_plugin_config* pc = (my_plugin_config*) plugin_config;
      zepto_set_pin(pc->request_pin_number,0);
      ps->state = 0;
    }

    //TODO: reinit? (via deinit, or directly, or implicitly)

    byte my_plugin_handler(const void* plugin_config, void* plugin_state,
      ZEPTO_PARSER* command, REPLY_HANDLE reply, WAITING_FOR* waiting_for) {
      const my_plugin_config* pc = (my_plugin_config*) plugin_config;
      my_plugin_state* ps = (my_plugin_state*)plugin_state;

      switch(ps->state) {
        case 0:
          //requesting sensor to perform read, using pc->request_pin_number
          zepto_set_pin(pc->request_pin_number,1);

          //waiting for sensor to indicate that data is ready
          zepto_indicate_waiting_for_pin(waiting_for,pc->ack_pin_number,1);
          return WAITING_FOR;

        case 1:
          uint16_t data_read = zepto_read_from_pins(pc->reply_pin_numbers,4);
          zepto_reply_append_byte(reply,data_read);
          return 0;

        default:
          assert(0);
      }
    }

Such an approach allows SmartAnthill implementation (such as Zepto VM) to perform proper pausing (with ability for SmartAnthill Client to interrupt processing by sending a new command while it didn't receive an answer to the previous one), when long waits are needed. It also enables parallel processing of the plugins (see PARALLEL instruction of Zepto VM in :ref:`sazeptovm` document for details).

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

TODO: WAITING_FOR

TODO: half-float library

Functions
^^^^^^^^^

Names of all functions within plugin interface start from ``papi_`` using by plugins of any functions with names not starting from ``papi_`` is not supported. All such calls should be declared in a single papi.h file, and, if possible, this file should not include any other file listing function calls (that is, calls not related to plugin API).

Parsing request and writing response functions
''''''''''''''''''''''''''''''''''''''''''''''

Request parsing functions:
""""""""""""""""""""""""""

.. function:: uint8_t papi_parser_read_byte( ZEPTO_PARSER* po );
.. function:: uint16_t papi_parser_read_encoded_uint16( ZEPTO_PARSER* po );
.. function:: uint16_t papi_parser_read_encoded_signed_int16( ZEPTO_PARSER* po );
.. function:: TODO: add vector-related functions

Writing functions:
""""""""""""""""""

.. function:: void papi_reply_write_byte( REPLY_HANDLE mem_h, uint8_t val );
.. function:: void papi_reply_write_encoded_uint16( REPLY_HANDLE mem_h, uint16_t num );
.. function:: void papi_reply_write_encoded_signed_int16( REPLY_HANDLE mem_h, int16_t sx );
.. function:: TODO: add vector-related functions

Misc functions:
"""""""""""""""
.. function:: void papi_init_parser_with_parser( ZEPTO_PARSER* po, const ZEPTO_PARSER* po_base );
.. function:: bool papi_parser_is_parsing_done( ZEPTO_PARSER* po );
.. function:: uint16_t papi_parser_get_remaining_size( ZEPTO_PARSER* po );


EEPROM access
'''''''''''''

.. function:: bool papi_eeprom_write( uint16_t plugin_id, const uint8_t* data );
.. function:: bool papi_eeprom_read( uint16_t plugin_id, uint8_t* data );

   plugin_id should eventually be converted to slot_id; data_size must be declared by plugin writer in advance (that is, in plugin manifest); mapping of plugin_id to slot_id must be done at time of firmware code generation (exact details are TBD).

.. function:: void papi_eeprom_flush();

   when this function returns, results of previous 'write' operations are guaranteed to be actually stored in eeprom. Note: depending on a particular archetecture this may result in an actually-empty call.


Non-blocking calls to access hardware
'''''''''''''''''''''''''''''''''''''

Here are calls to access pins.

.. function:: bool papi_read_digital_pin( uint16_t pin_num );
.. function:: void papi_write_digital_pin( uint16_t pin_num, bool value );

The following calls implement access to devices sitting behind SPI and I2C interfaces. Each size is in bits. TODO: discuss the order of bits within an unsigned int representing command/data

.. function:: void papi_start_sending_spi_command_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_start_sending_spi_command_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);

.. function:: void papi_start_sending_i2c_command_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_start_sending_i2c_command_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);

   Each of the above papi_start_sending_*() calls start an operation and return immediately; to know that the request is already performed wait for a respective spi_id / i2c_id

.. function:: uint8_t papi_start_receiving_spi_data_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_start_receiving_spi_data_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

.. function:: uint8_t papi_start_receiving_i2c_data_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_start_receiving_i2c_data_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

   Each of the above papi_start_receiving_*() calls start an operation and return immediately; to know that the data is already available wait for a respective spi_id / i2c_id

.. function:: uint8_t papi_cancel_spi_operation( uint8_t spi_id );
.. function:: uint8_t papi_cancel_i2c_operation( uint8_t spi_id );

   Each of the above ``papi_cancel_*()`` calls return immediately. TODO: do we need to supply as parameters addr and addr_sz as well?


Blocking calls
''''''''''''''

All calls in this group are pseudo-functions that will be compiled to a proper sequence of calls that implements initiating of a correspondent operation and starting waiting for the result.

Blocking calls to to wait for timeout
"""""""""""""""""""""""""""""""""""""

.. function:: void papi_sleep( uint16_t millisec );//TODO: time instead of ms?

Blocking calls to access hardware
"""""""""""""""""""""""""""""""""

.. function:: void papi_wait_for_sending_spi_command_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_spi_command_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_i2c_command_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_i2c_command_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);
.. function:: uint8_t papi_wait_for_receiving_spi_data_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_wait_for_receiving_spi_data_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);
.. function:: uint8_t papi_wait_for_receiving_i2c_data_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_wait_for_receiving_i2c_data_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

.. function:: uint8_t papi_wait_for_wait_handler( WAITING_FOR* wf );//see helper functions below

Helper functions to fill WAITING_FOR structure
''''''''''''''''''''''''''''''''''''''''''''''

.. function:: papi_init_wait_handler( WAITING_FOR* wf );
.. function:: papi_wait_handler_add_wait_for_spi_send( WAITING_FOR* wf, uint8_t spi_id );
.. function:: papi_wait_handler_add_wait_for_i2c_send( WAITING_FOR* wf, uint8_t i2c_id );
.. function:: papi_wait_handler_add_wait_for_spi_receive( WAITING_FOR* wf, uint8_t spi_id );
.. function:: papi_wait_handler_add_wait_for_i2c_receive( WAITING_FOR* wf, uint8_t i2c_id );
.. function:: papi_wait_handler_add_wait_for_timeout( WAITING_FOR* wf, SA_TIME_VAL tv );

.. function:: bool papi_wait_handler_is_waiting( WAITING_FOR* wf );

   TODO: think about parameters

Yet unsorted calls
''''''''''''''''''

.. function:: void papi_gravely_power_inefficient_micro_sleep( SA_TIME_VAL* timeval );

