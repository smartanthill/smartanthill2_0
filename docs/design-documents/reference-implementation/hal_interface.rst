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

.. _hal_interface:

HAL interface
=============

**for preliminary discussion**

:Version:   v0.0.0

HAL interface consists of waiting and time, communication, and EEPROM access functions (TODO: any other?)

Data structures
---------------

* WAITING_FOR

   describes various objects/events that can be waited; respectively, must address all of such objects (in particular, SPI, I2C, etc; details are TBD and is as follows)

* SA_TIME_VAL

   expresses time in a way understood by HAL; a set of converting functions (or macros) must be provided to go back and force between SA_TIME_VAL and usual time units (such as seconds), and to perform necessary operations. Internal structure of SA_TIME_VAL must be of no interest to OS.

Initializing functions
----------------------

.. function:: void hal_init();

   performs whatever initialization after system reboot (communication, eeprom, etc) from hardware point of view (for instance, may include functionality of a presently existing communication_initialize(), hal_init_eeprom_access(), etc).

Waiting functions
-----------------

.. function:: uint8_t hal_wait_for( WAITING_FOR* wf );

   waiting for objects described in WAITING_FOR struct. Returns when one or more events heppen.

.. function:: void hal_mcu_sleep( uint16_t sec, uint8_t transmitter_state_on_exit );

   causes MCU to go to sleep mode and returns at the end of this period; no other processing is done until this function retuens (for obvious reason). TBD: way to supply time interval (seconds vs. SA_TIME_VAL)

.. function:: void hal_gravely_power_inefficient_micro_sleep( SA_TIME_VAL* timeval );

   presently named ``just_sleep( SA_TIME_VAL* timeval )``, a blocking call; a time interval should not be permitted to be somehow substantial (say, in order of 1 ms max)


Time functions
--------------

.. function:: void hal_get_time( SA_TIME_VAL* t );

   fills SA_TIME_VAL struct; the result may then be used by supplying as is in a various time and wait related functions

* TODO: add time conversion and other related functions/macros


Communication functions
-----------------------

.. function:: uint8_t hal_send_packet( MEMORY_HANDLE mem_h, uint8_t bus_id, uint8_t intrabus_id );

   TODO: think about 'NOT_USED' value for intrabus_id

.. function:: bool hal_get_packet_bytes( MEMORY_HANDLE mem_h );

   used for actual getting of the packet bytes (for instance, when hal_wait_for indicates that packet bytes are available). Used in repeated way together with hal_wait_for; returns true, when the whole packet is received. Note: by definition, packet ends when this call returns true; whether packet is integral will be checkedbeyond HAL.

.. function:: void hal_turn_receiver_on_off( bool turn_on );

   does nothing if receiver is already in a requested state


EEPROM access functions
-----------------------

.. function:: bool hal_eeprom_write( const uint8_t* data, uint16_t size, uint16_t address );

   self-described.

.. function:: bool hal_eeprom_read( uint8_t* data, uint16_t size, uint16_t address);

   self-described.

.. function:: void hal_eeprom_flush();

   when this function returns, results of previous 'write' operations are guaranteed to be actually stored in eeprom. Note: depending on a particular archetecture this may be an empty call.


Digital pin operation functions
-------------------------------

.. function:: bool hal_read_digital_pin( uint16_t pin_num );
.. function:: void hal_write_digital_pin( uint16_t pin_num, bool value );


SPI and I2C operation functions
-------------------------------

In the following calls each size is in bits. TODO: discuss the order of bits within an unsigned int representing command/data

.. function:: void hal_start_sending_spi_command_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void hal_start_sending_spi_command_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);

.. function:: void hal_start_sending_i2c_command_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t command, uint8_t command_sz);
.. function:: void hal_start_sending_i2c_command_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t command, uint8_t command_sz);

   Each of the above ``hal_start_sending_*()`` calls start an operation and return immediately; (if at all possible) to know that the request is already performed caller should wait for a respective spi_id / i2c_id by calling hal_wait_for(), and hal_wait_for() should return as soon as HAL finds the operation is over.

.. function:: uint8_t hal_start_receiving_spi_data_16( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t hal_start_receiving_spi_data_32( uint8_t spi_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

.. function:: uint8_t hal_start_receiving_i2c_data_16( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint16_t* data);
.. function:: uint8_t hal_start_receiving_i2c_data_32( uint8_t i2c_id, uint16_t addr, uint8_t addr_sz, uint32_t* data);

   Each of the above ``hal_start_receiving_*()`` calls start an operation and return immediately; to know that the data is already received caller should wait for a respective spi_id / i2c_id by calling hal_wait_for(), and hal_wait_for() should return as soon as HAL finds the operation is over.

.. function:: uint8_t hal_cancel_spi_operation( uint8_t spi_id );
.. function:: uint8_t hal_cancel_i2c_operation( uint8_t spi_id );

   Each of the above ``hal_cancel_*()`` calls return immediately. TODO: do we need to supply as parameters addr and addr_sz as well?



Special TX/RX functions
-----------------------

.. function:: bool hal_set_frequency();

   parameters? ret?

.. function:: void hal_adjust_transmitting_power( bool increase );

   Increases or decreases transmitting power; decrease is done when possible; increase is done until max possible power is reached.

.. function:: void hal_set_max_transmitting_power();
.. function:: int8_t hal_get_min_max_transmitting_power();

   If we need a range, think about returning a pair, or about splitting this call into two (for min and max).

.. function:: bool hal_is_frequency_adjustable();
.. function:: void hal_adjust_frequency();

   Input parameters?

