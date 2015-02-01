v0.2.3a


SmartAnthill Reference Implementation - MCU Software Architecture
=================================================================

*NB: this document relies on certain terms and concepts introduced in “SmartAnthill Overall Architecture” document, please make sure to read it before proceeding.*
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

SmartAnthill project is intended to operate on MCUs which are extremely resource-constrained. In particular, currently we're aiming to run SmartAnthill on MCUs with as little as 512 bytes of RAM. This makes the task of implementing SmartAnthill on such MCUs rather non-trivial. Present document aims to describe our approach to implementing SmartAnthill on MCU side. It should be noted that what is described here is merely our approach to a reference SmartAnthill implementation; it is not the only possible approach, and any other implementation which is compliant with SmartAnthill protocol specification, is welcome (as long as it can run on target MCUs and meet energy consumption and other requirements).

.. contents::

Assumptions (=mother of all screw-ups)
--------------------------------------

1. RAM is the most valuable resource we need to deal with. Limits on RAM are on the order of 512-8192 bytes, while limits on code size are on the order of 8192-32768 bytes.
2. EEPROM (or equivalent) is present on all the supported chips
3. There is a limit on number of EEPROM operations (such as 10000 to 100000 during MCU lifetime, depending on MCU)
4. This limit is usually per-writing-location and EEPROM writings are done with some granularity which is less than whole EEPROM size. One expected granularity size is 32 bits-per-write; if EEPROM on MCU has such a granularity, it means that even we're writing one byte, we're actually writing 4 bytes (and reducing the number of available writes for all 4 bytes).
5. There are MCUs out there which allow to switch to “sleep” mode
6. During such “MCU sleep”, RAM may or may not be preserved (NB: if RAM is preserved, it usually means higher energy consumption)
7. During such “MCU sleep”, receiver may or may not be turned off (NB: this issue is addressed in detail in "SmartAnthill Protocol Stack" and "SAGDP" documents).

Layers and Libraries
--------------------

Reference implementation of SmartAnthill on MCU is divided into two parts:

* SmartAnthill reference implementation as such (the same for all MCUs)
* MCU- and device-dependent libraries

Memory Architecture
-------------------

As RAM is considered the most valuable resource, it is suggested to pay special attention to RAM usage. 

SmartAnthill memory architecture is designed as follows:

* Two large blocks of RAM are pre-allocated: a) stack (TODO: size), b) “command buffer”
* “command buffer” is intended to handle variable-size request/response data (such as incoming packets, packets as they're processed by protocol handlers, replies etc.); it's use is described in detail in "Main Loop" section below.
* In addition, there are fixed-size states of various state machines which implement certain portions of SmartAnthill protocol stack (see details below). These fixed-size state may either reside globally, or on stack of "main loop" (see below)

"Main Loop" a.k.a. "Main Pump"
------------------------------

SmartAnthill on MCU is implemented as a "main loop", which calls different functions and performs other tasks as follows:

* first, "main loop" calls a function [TODO](void\* data, uint16 datasz), which waits for an incoming packet and fills *data* with an incoming packet. This function is a part of device-specific library. If incoming packets can arrive while the "main loop" is running, i.e. asynchronously, they need to be handled in a separate buffer and handled separately, but otherwise "main loop" can pass a pointer to the beginning of the “command buffer” to this function call.
* then, "main loop" calls one “receiving” protocol handler (such as “receiving” portion of SADLP-CAN), with the following prototype: **protocol_handler(const void\* src,uint16 src_size,void\* dst, uint16\* dst_size);** In fact, for this call "main loop" uses both *src* and *dst* which reside within “command buffer”, *src* coming from the “command buffer” start, and *dst=src+src_size*.
* NB: all calls of protocol handlers (both “receiving” and “sending”) are made right from the program “main loop” (and not one protocol handler calling another one), to reduce stack usage.
* after protocol handler has processed the data, it returns to “main loop”. Now previous src is not needed anymore, so "main loop" can and should **memmove()** dst to the beginning of “command buffer”, discarding src and freeing space in "command buffer" for future processing.
* after such **memmove()** is done, we have current packet (as processed by previous protocol handler) at the beginning of “command buffer”, so we can repeat the process of calling the “receiving” “protocol handler” (such as SAGDP, and then Yocto VM).
* when Yocto VM is called (it has prototype **yocto_vm(const void\* src,uint16 src_size,void\* dst, uint16\* dst_size,WaitingFor\* waiting_for);**; *WaitingFor* structure is described in detail in 'Asynchronous Returns' subsection below), it starts parsing the command buffer and execute commands. Whenever Yocto VM encounters an EXEC command (see "Yocto VM" document for details), Yocto VM calls an appropriate plugin handler, with the following prototype: **plugin_handler(const void\* plugin_config, void\* plugin_state, const void\* cmd, uint16 cmd_size, REPLY_HANDLE reply, WaitingFor\* waiting_for)**, passing pointer to plugin data as a cmd and creating *REPLY_HANDLE reply* out of it's own *dst*. See details on REPLY_HANDLE in 'Plugin API' section below. After plugin_handler returns, Yocto VM makes sure that it's own *dst* is incremented by a size of the accumulated reply. This ensures proper and easy forming of "reply buffer" as required by Yocto VM specification.
* after the Yocto VM has processed the data, “main loop” doesn't need the command anymore, so it can again **memmove()** "reply buffer" (returned at *dst* location by Yocto VM) to the beginning of “command buffer” and call SAGDP “sending” protocol handler.
* after “sending” protocol handler returns, “main loop” may and should **memmove()** reply of the “sending” protocol handler to the beginning of the “command buffer” and continue calling the “sending” protocol handlers (and memmove()-ing data to the beginning of the “command buffer”) until the last protocol handler is called; at this point, data is prepared for feeding to the physical channel.
* at this point, "main loop" may and should call [TODO] function (which belongs to device-specific library) to pass data back to the physical layer.

In a sense, "main loop" is always "pumping" the data from one "protocol handler" to another one, always keeping "data to be processed" in the beginning of the "command buffer" and discarding it as soon as it becomes unnecessary. This "pumping" **memmove()**-based approach allows to avoid storing multiple copies of data (only two copies are stored at any given moment), and therefore to save on the amount of RAM required for SmartAnthill stack operation.

Return Codes
^^^^^^^^^^^^

Each protocol handler returns error code. Error codes are protocol-handler specific and may include such things as IGNORE_PACKET (causing "main loop" to stop processing of current packet and start waiting for another one), FATAL_ERROR_REINIT (causing "main loop" to perform complete re-initialization of the whole protocol stack), WAITING_FOR (described below in 'Asynchronous Returns' subsection) and so on.

Asynchronous Returns from Yocto VM 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to paramaters which are usual for protocol handlers, Yocto VM also receives a pointer to a struct WaitingFor { uint16 sec; uint16 msec; byte pins_to_wait[(NPINS+7)/8]; byte pin_values_to_wait[(NPINS+7)/8] };
When Yocto VM execution is paused to wait for some event, it SHOULD return to "main loop" with an error code = WAITING_FOR, filling in this parameter with time which it wants to wait, and filling in any pins (with associated pin values) for which it wants to wait. These instructions to wait for are always treated as waiting for *any* of conditions to happen, i.e. to "wait for time OR for pin#2==1 OR for pin#4==0".

It is responsibility of the "main loop" to perform waiting as requested by Yocto VM and call it back when the condition is met (passing NULL for src). 

During such a wait, "main loop" is supposed to wait for incoming packets too; if an incoming packet comes in during such a wait, "main loop" should handle incoming packet first (before reporting to 'Yocto VM' that it's requested wait is over). 

Yocto VM may issue WAITING_FOR either as a result of SLEEP instruction, or as a result of plugin handler returning WAITING_FOR (see example below).

TODO: MCUSLEEP?

State Machines
--------------

Model which is described above in "Main Loop" section, implies that all SmartAnthill protocol handlers (including Yocto VM) are implemented as "state machines"; state of these "state machines" should be fixed-size and belongs to "fixed-size states" memory area mentioned in "Memory Architecture" section above.

Plugins
^^^^^^^

Ideally, plugins SHOULD also be implemented as state machines, for example:

::

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
      const void* cmd, uint16 cmd_size, REPLY_HANDLE reply, WaitingFor* waiting_for) {
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
  }

Such an approach allows Yocto VM to perform proper pausing (with ability for Central Controller to interrupt processing by sending a new command while it didn't receive an answer to the previous one) when long waits are needed. It also enables parallel processing of the plugins (TODO: PARALLEL instruction for Yocto VM).

However, for some plugins (simple ones without waiting at all, or if we're too lazy to write proper state machine), we can use 'dummy state machine', with *MyPluginState* being NULL and unused, and **plugin_handler()** not taking into account any states at all.


Programming Guidelines
----------------------

The following guidelines are considered important to ensure that only absolutely minimum amount of RAM is used:

* Dynamic allocation is not used, at all. (yes, it means no **malloc()**)
* No third-party libraries (except for those specially designed for MCUs) are allowed
* All on-stack arrays MUST be analyzed for being necessary and rationale presented in comments.

Support for PARALLEL instruction
--------------------------------

PARALLEL instruction is supported starting from YoctoVM-Medium. It allows for pseudo-parallel execution (i.e. when plugin A is waiting, plugin B may continue to work). 

Implementing PARALLEL instruction is tricky, in particular, because we don't know how much space to allocate for each pseudo-thread to use from "reply buffer". To get around this problem, we've encapsulated reply buffer as an opaque YOCTOVM_REPLYBUFFER handle, which allows us to move reply sub-buffers as it is needed as the pseudo-threads are working and plugins are requesting 'push_reply(handle)'.

In addition, to accommodate per-pseudo-thread expression stacks, at the moment of PARALLEL instruction we perform a 'virtual split' of the remaining space in "expression stack" into "per-pseudo-thread expression stacks"; to implement this 'virtual split', we keep an array of offsets of these "per-pseudo-thread expression stacks" within main "expression stack", and move them as necessary to accommodate expression stack requests (in a manner similar to the handling of "reply sub-buffers" described above).

EEPROM Handling
---------------

TODO

Plugin API
----------

Yocto VM provides certain APIs for plugins. 

Data Types
^^^^^^^^^^

REPLY_HANDLE
''''''''''''

REPLY_HANDLE is an encapsulation of a "reply buffer", which allows plugin to call **reply_append()** (see below). 
REPLY_HANDLE is normally obtained as a parameter from plugin_handler() call.

**Caution:** Plugins MUST treat REPLY_HANDLE as completely opaque and MUST NOT try to use it to access reply buffer directly; doing so may easily result in memory corruption when running certain Yocto VM programs (for example, when PARALLEL instruction is used).

TODO: WaitingFor

Functions
^^^^^^^^^

reply_append()
''''''''''''''

**void\* reply_append(REPLY_HANDLE handle,uint16 sz);**

reply_append() allocates 'sz' bytes within "reply buffer" specified by handle and returns a pointer to this allocated buffer. This buffer can be then filled with plugin's reply. 

**Caution:** note that the pointer returned by reply_append() is temporary and may change between different calls to the same plugin, i.e. this pointer (or derivatives) MUST NOT be stored as a part of the plugin state.

TODO: describe error conditions (such as lack of space in buffer)

