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

.. _sazeptoos:

SmartAnthill Zepto OS
=====================

:Version:   v0.2.6

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill project is intended to operate on MCUs which are extremely resource-constrained. In particular, currently we're aiming to run SmartAnthill on MCUs with as little as 512 bytes of RAM. This makes the task of implementing SmartAnthill on such MCUs rather non-trivial. Present document aims to describe our approach to implementing SmartAnthill on MCU side, named 'Zepto OS'. It should be noted that what is described here is merely our approach to a reference SmartAnthill implementation; it is not the only possible approach, and any other implementation which is compliant with SmartAnthill protocol specification, is welcome (as long as it can run on target MCUs and meet energy consumption and other requirements).

"Zepto OS" is a network-oriented secure operating system, intended to run on extremely resource-constrained devices. It implements necessary parts of :ref:`saprotostack`, including :ref:`sazeptovm`.

“Zepto” is a prefix in the metric system (SI system) which denotes a factor of 10^-21. This is 10^12 times less than “nano”, a billion times less than “pico”, and a million times less than “femto”. As of now, 'zepto' is the second smallest prefix in SI system (we didn't take the smallest one, because there is always room for improvement).

Zepto VM is the smallest OS we were able to think about, with an emphasis of using as less RAM as possible. Nevertheless, it has network support (more specifically, it supports necessary parts of :ref:`saprotostack`), strong encryption support (both AES-128 and Speck), and "green threads". All of this in as low as 512 bytes of RAM. To achieve it, Zepto OS has quite a specific design.


.. contents::

Assumptions (=mother of all screw-ups)
--------------------------------------

1. RAM is the most valuable resource we need to deal with. Limits on RAM are on the order of 512-8192 bytes, while limits on code size are on the order of 8192-32768 bytes.
2. EEPROM (or equivalent) is present on all the supported chips
3. There is a limit on number of EEPROM operations (such as 10000 to 100000 during MCU lifetime, depending on MCU)
4. This limit is usually per-writing-location and EEPROM writings are done with some granularity which is less than whole EEPROM size. One expected granularity size is 32 bits-per-write; if EEPROM on MCU has such a granularity, it means that even we're writing one byte, we're actually writing 4 bytes (and reducing the number of available writes for all 4 bytes).
5. There are MCUs out there which allow to switch to “sleep” mode
6. During such “MCU sleep”, RAM may or may not be preserved (NB: if RAM is preserved, it usually means higher energy consumption)
7. During such “MCU sleep”, receiver may or may not be turned off (NB: this issue is addressed in detail in :ref:`saprotostack` and :ref:`sagdp` documents).

Layers and Libraries
--------------------

Zepto OS is divided into three parts:

* Zepto OS kernel (the same for all MCUs)
* MCU- and device-dependent libraries (Hardware Abstraction Layer, HAL)
* SmartAnthill Plugins (see :ref:`saplugin` document; from the point of view of Zepto OS plugins are similar to device drivers).

Memory Architecture
-------------------

As RAM is considered the most valuable resource, it is suggested to pay special attention to RAM usage.

SmartAnthill memory architecture is designed as follows:

* Two large blocks of RAM are pre-allocated: a) stack (TODO: size), b) “Zepto Heap"
* In addition, there are fixed-size states of various state machines which implement certain portions of SmartAnthill protocol stack (see details below). These fixed-size state may either reside globally, or on stack of "main loop" (see below)

Zepto Heap
^^^^^^^^^^

Zepto Heap provides access to memory allocation. However, to enable work within extremely tight memory constraints, "Zepto Heap" is unusual in a sense that all memory blocks are movable. Therefore, pointers to these memory blocks may easily change, when calling various functions (but not between such calls). As a rule of thumb, any potentially "blocking" or "allocating" function MAY change all the pointers; to avoid problems, all the pointers MUST be re-read from respective handles after each such function call. Outside of Zepto OS the API is built in a way that this requirement is usually not a problem, as long as handles are treated as completely opaque. TODO: describe exceptions if any

Each memory block within Zepto Heap is represented by REQUEST_REPLY_HANDLE. REQUEST_REPLY_HANDLE provides parsing and appending functionality which is described in :ref:`saplugin` document. Other functionality provided by REQUEST_REPLY_HANDLE:

**void zepto_convert_reply_to_request(REQUEST_REPLY_HANDLE);**

zepto_convert_reply_to_request() function discards request within REQUEST_REPLY_HANDLE, and converts reply contained within it, into request. This function is used in "main loop" as described below.

Data which corresponds to REQUEST_REPLY_HANDLEs is stored in a global "heap control" structure; maximum number of simultaneously supported REQUEST_REPLY_HANDLEs is limited to a compile-time-constant ZEPTO_MAX_HEAP_BLOCKS.

Whenever 'append' doesn't fit into free memory right after the block being appended to, Zepto Heap moves blocks (the block being appended to, other blocks, or any combination of these) to allow appending. This in turn causes pointers to these blocks to be invalidated (as noted above).

REPLY_HANDLE
''''''''''''

Plugin APIs are using REPLY_HANDLEs; REPLY_HANDLEs are implemented as REQUEST_REPLY_HANDLEs with limited functionality (i.e. no parser can be created from REPLY_HANDLE, but zepto_append_*() functions do work for REPLY_HANDLEs). 

ZEPTO_PARSER
''''''''''''

ZEPTO_PARSER is an opaque structure which is used for parsing packets. It is also used by SmartAnthill Plugins (as described in :ref:`saplugin` document). In addition to functions described in :ref:`saplugin` document, ZEPTO_PARSER supports the following functionality:

**void zepto_create_parser(ZEPTO_PARSER* parser, REQUEST_REPLY_HANDLE request_reply);**

zepto_create_parser() function initializes ZEPTO_PARSER structure and prepares it for subsequent use (similar to OO constructor).

**void zepto_create_parser_from_parsed_block(ZEPTO_PARSER* target_parser, ZEPTO_PARSER* source_parser, size_t sz);**

zepto_create_parser_from_parsed_block() initializes a new ZEPTO_PARSER from a block of size sz within existing parser (similar to another OO constructor). This is used to support nested parsing (which in turn enables plugin processing as described below).

Error Handling and Zepto Exceptions
-----------------------------------

In Zepto OS, errors are normally handled via "Zepto Exceptions". Zepto exceptions is a series of macros, which are implemented either via setjmp/longjmp (if it is present on target MCU), or without them. 

Zepto exception macros are used as follows:

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

Intermediate processing (MUST be written whenever a function-able-to-throw-exception is called; necessary to handle platforms where setjmp/longjmp is not available):

.. code-block:: c

  function_able_to_throw_exception();
  ZEPTO_UNWIND(-1); //returns '-1' in case of exception unwinding

"Main Loop" a.k.a. "Main Pump"
------------------------------

Zepto OS is implemented as a "main loop", which calls different functions and performs other tasks as follows:

* first, "main loop" calls a function zepto_hal_incoming_packet(REQUEST_REPLY_HANDLE data), which waits for an incoming packet and fills *data* with an incoming packet. This function is a part of device-specific library. If incoming packets can arrive while the "main loop" is running, i.e. asynchronously, they need to be handled in a separate buffer and handled separately. 
* then, "main loop" calls one of “receiving” protocol handlers (such as “receiving” portion of SADLP-CAN), with the following prototype: **byte protocol_handler(REQUEST_REPLY_HANDLE);** 
* NB: all calls of protocol handlers (both “receiving” and “sending”) are made right from the program “main loop” (and not one protocol handler calling another one), to reduce stack usage.
* after protocol handler has processed the data, it returns to “main loop”. Now previous request within REQUEST_REPLY_HANDLE is not needed anymore, so "main loop" calls zepto_convert_reply_to_request() to discard previous request and to convert reply of the previous protocol layer into request of the next protocol layer.
* after such zepto_convert_reply_to_request() call, we can repeat the process of calling the “receiving” “protocol handler” (such as SAGDP, and then SACCP and Zepto VM).
* when Zepto VM is called (it has prototype **zepto_vm(REQUEST_REPLY_HANDLE, WaitingFor\* waiting_for);**; *WaitingFor* structure is described in detail in 'Asynchronous Returns' subsection below), it starts parsing the request and execute commands. Whenever Zepto VM encounters an EXEC command (see :ref:`sazeptovm` document for details), Zepto VM creates a nested ZEPTO_PARSER (to parse plugin data), and calls an appropriate plugin handler (passing this nested parser as a parameter); prototype of plugin handler is specified in :ref:`saplugin` document. After plugin_handler returns, Zepto VM merges plugin reply into it's own reply. This ensures proper and easy forming of "reply buffer" as required by Zepto VM specification.
* after the Zepto VM has processed the data, “main loop” doesn't need the command anymore, so it can again call zepto_convert_reply_to_request() and call SAGDP “sending” protocol handler.
* after “sending” protocol handler returns, “main loop” calls zepto_convert_reply_to_request() and continues calling the “sending” protocol handlers (and zepto_convert_reply_to_request() after each protocol handler call) until the last protocol handler is called; at this point, data is prepared for feeding to the physical channel.
* at this point, "main loop" calls [TODO] function (which belongs to device-specific library) to pass data back to the physical layer.

In a sense, "main loop" is always "pumping" the data from one "protocol handler" to another one, always keeping "data to be processed" within the same REQUEST_REPLY_HANDLE, calling zepto_convert_reply_to_request() (which effectively discards 'old' request data and converts reply data into 'new' request data) as soon as 'old' request data becomes unnecessary. This "pumping" zepto-convert_reply_to_request()-based approach allows to avoid storing multiple copies of data (only two copies are stored at any given moment), and therefore to save on the amount of RAM required for SmartAnthill stack operation.

Return Codes
^^^^^^^^^^^^

Each protocol handler returns error code. Error codes are protocol-handler specific and may include such things as IGNORE_PACKET (causing "main loop" to stop processing of current packet and start waiting for another one), FATAL_ERROR_REINIT (causing "main loop" to perform complete re-initialization of the whole protocol stack), WAITING_FOR (described below in 'Asynchronous Returns' subsection) and so on.

Asynchronous Returns from Zepto VM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to paramaters which are usual for protocol handlers, Zepto VM also receives a pointer to a struct WaitingFor { uint16_t sec; uint16_t msec; byte pins_to_wait[(NPINS+7)/8]; byte pin_values_to_wait[(NPINS+7)/8] };
When Zepto VM execution is paused to wait for some event, it SHOULD return to "main loop" with an error code = WAITING_FOR, filling in this parameter with time which it wants to wait, and filling in any pins (with associated pin values) for which it wants to wait. These instructions to wait for are always treated as waiting for *any* of conditions to happen, i.e. to "wait for time OR for pin#2==1 OR for pin#4==0".

It is responsibility of the "main loop" to perform waiting as requested by Zepto VM and call it back when the condition is met (passing NULL for src).

During such a wait, "main loop" is supposed to wait for incoming packets too; if an incoming packet comes in during such a wait, "main loop" should handle incoming packet first (before reporting to 'Zepto VM' that it's requested wait is over).

Zepto VM may issue WAITING_FOR either as a result of SLEEP instruction, or as a result of plugin handler returning WAITING_FOR (see example below).

TODO: MCUSLEEP?

State Machines
--------------

Model which is described above in "Main Loop" section, implies that all SmartAnthill protocol handlers (including Zepto VM) are implemented as "state machines"; state of these "state machines" should be fixed-size and belongs to "fixed-size states" memory area mentioned in "Memory Architecture" section above.

Plugins
-------

Zepto OS plugins MUST be compliant with SmartAnthill Plugin specification, as outlined in :ref:`saplugin` document.

Programming Guidelines
----------------------

The following guidelines are considered important to ensure that only absolutely minimum amount of RAM is used:

* Dynamic allocation is heavily discouraged. When used, it MUST be based on REQUEST_REPLY_HANDLES as described above (yes, it means no **malloc()**)
* No third-party libraries (except for those specially designed for MCUs) are allowed
* All on-stack arrays MUST be analyzed for being necessary and rationale presented in comments.

Support for PARALLEL Zepto VM instruction
-----------------------------------------

PARALLEL instruction is supported by Zepto VM, starting from ZeptoVM-Medium. It allows for pseudo-parallel execution (i.e. when plugin A is waiting, plugin B may continue to work).

Implementing PARALLEL instruction is tricky, in particular, because we don't know how much space to allocate for each pseudo-thread to use from "reply buffer". To get around this problem, we've encapsulated reply buffer as an opaque REQUEST_REPLY_HANDLE. 

In addition, to accommodate per-pseudo-thread expression stacks, at the moment of PARALLEL instruction we perform a 'virtual split' of the remaining space in "expression stack" into "per-pseudo-thread expression stacks"; to implement this 'virtual split', we keep an array of offsets of these "per-pseudo-thread expression stacks" within main "expression stack", and move them as necessary to accommodate expression stack requests (in a manner similar to the handling of "reply sub-buffers" described above).

EEPROM Handling
---------------

TODO

Running on top of another OS
----------------------------

Zepto OS is written in generic C code, and can be compiled and run as an application on top of another OS, as long as Zepto OS HAL is implemented. As of now, Zepto OS can run on top of Windows, we also plan to add support for Linux and Mac OS X.

Hardware Abstraction Layer (HAL)
--------------------------------

HAL is intended to enable Zepto OS to run on different architectures. Below is the list of functions which HAL needs to provide:

TODO: error codes

**int zepto_hal_incoming_packet(REQUEST_REPLY_HANDLE data);**

where bufSize is an inout parameter, taking original buffer size and returning packet size back. get_incoming_packet() returns error code (TODO: codes)

TODO: more and more and more


