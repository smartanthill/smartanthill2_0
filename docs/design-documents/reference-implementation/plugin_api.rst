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

.. _papi:

Plugin API
==========

**for preliminary discussion**

:Version:   v0.0.0

Plugin interface functions are to solve the following problems:

* Parsing request and writing response;
* Perform hardware access;
* perform waiting for various objects/events;
* organize EEPROM access.

Names of all functions within plugin interface start from ``papi_`` using by plugins of any functions with names not starting from ``papi_`` is not supported. All such calls should be declared in a single papi.h file, and, if possible, this file should not include any other file listing function calls (that is, calls not related to plagin API).


Parsing request and writing response functions
----------------------------------------------

Request parsing function:
^^^^^^^^^^^^^^^^^^^^^^^^^

.. function:: uint8_t papi_parser_read_byte( parser_obj* po );
.. function:: uint16_t papi_parser_read_encoded_uint16( parser_obj* po );
.. function:: uint16_t papi_parser_read_encoded_signed_int16( parser_obj* po );
.. function:: TODO: add vector-related functions

Writing functions:
^^^^^^^^^^^^^^^^^^

.. function:: void papi_reply_write_byte( REPLY_HANDLE mem_h, uint8_t val );
.. function:: void papi_reply_write_encoded_uint16( REPLY_HANDLE mem_h, uint16_t num );
.. function:: void papi_reply_write_encoded_signed_int16( REPLY_HANDLE mem_h, int16_t sx );
.. function:: TODO: add vector-related functions

Misc functions:
^^^^^^^^^^^^^^^
.. function:: void papi_init_parser_with_parser( parser_obj* po, const parser_obj* po_base );
.. function:: bool papi_parser_is_parsing_done( parser_obj* po );
.. function:: uint16_t papi_parser_get_remaining_size( parser_obj* po );


EEPROM access
-------------

.. function:: bool papi_eeprom_write( uint16_t plugin_id, const uint8_t* data );
.. function:: bool papi_eeprom_read( uint16_t plugin_id, uint8_t* data );

   plugin_id should eventually be converted to slot_id; data_size must be declared by plugin writer in advance (that is, in plugin manifest); mapping of plugin_id to slot_id must be done at time of firmware code generation (exact details are TBD).

.. function:: void papi_eeprom_flush();

   when this function returns, results of previous 'write' operations are guaranteed to be actually stored in eeprom. Note: depending on a particular archetecture this may result in an actually-empty call.


Non-blocking calls to access hardware
-------------------------------------

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
--------------

All calls in this group are pseudo-functions that will be compiled to a proper sequence of calls that implements initiating of a correspondent operation and starting waiting for the result.

Blocking calls to access hardware
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. function:: void papi_wait_for_sending_spi_command_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_spi_command_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_i2c_command_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void papi_wait_for_sending_i2c_command_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);
.. function:: uint8_t papi_wait_for_receiving_spi_data_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_wait_for_receiving_spi_data_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);
.. function:: uint8_t papi_wait_for_receiving_i2c_data_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t papi_wait_for_receiving_i2c_data_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

Blocking calls to to wait for timeout
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. function:: void papi_sleep( uint16_t millisec );


Helper functions to fill WAITING_FOR structure
----------------------------------------------

.. function:: papi_init_wait_handler( WAITING_FOR* wf );
.. function:: papi_wait_handler_add_wait_for_spi_delivering_command( WAITING_FOR* wf, uint8_t spi_id );
.. function:: papi_wait_handler_add_wait_for_i2c_delivering_command( WAITING_FOR* wf, uint8_t i2c_id );
.. function:: papi_wait_handler_add_wait_for_spi_read( WAITING_FOR* wf, uint8_t spi_id );
.. function:: papi_wait_handler_add_wait_for_i2c_read( WAITING_FOR* wf, uint8_t i2c_id );
.. function:: papi_wait_handler_add_wait_for_timeout( WAITING_FOR* wf, SA_TIME_VAL tv );

.. function:: bool papi_wait_handler_is_waiting( WAITING_FOR* wf );

   TODO: think about parameters


Yet unsorted calls
------------------

.. function:: void papi_gravely_power_inefficient_micro_sleep( SA_TIME_VAL* timeval );
