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

.. _sazeptovm:

Zepto VM
========

:Version:   v0.2.12

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saccp` *documents, please make sure to read them before proceeding.*

Zepto VM is a minimalistic virtual machine used by SmartAnthill Devices. It implements SACCP (SmartAnthill Command&Control Protocol) on the side of the SmartAnthill Device (and SACCP corresponds to Layer 7 of OSI/ISO network model). By design, Zepto VM is intended to run on devices with extremely limited resources (as little as 512 bytes of RAM).

.. contents::

Sales pitch (not to be taken seriously!)
----------------------------------------

Zepto VM is the only VM which allows you to process fully-fledged Turing-complete byte-code, enables you to program your MCU the way professionals do, with all the bells and whistles such as flow control (including both conditions and loops), postfix expressions, subroutine calls, C routine calls, MCU sleep mode (where not prohibited by law of physics), and even a reasonable facsimile of “green threads” - all at a miserable price of 1 to 50 bytes of RAM (some restrictions apply, batteries not included). Yes, today you can get many of these features at the price of 1 (one) byte of RAM (offer is valid while supplies last, stores open late).

We're so confident in our product that we offer a unique memory-back guarantee for 30 days or 30 seconds, whichever comes first. Yes, if you are not satisfied with Zepto VM and remove it from your MCU, you'll immediately get all your hard earned bytes back, no questions asked, no strings attached.
TODO: proof of being Turing-complete via being able to implement brainfuck

Zepto VM Philosophy
-------------------

“Zepto” is a prefix in the metric system (SI system) which denotes a factor of 10^-21. This is 10^12 times less than “nano”, a billion times less than “pico”, and a million times less than “femto”. As of now, 'zepto' is the second smallest prefix in SI system (we didn't take the smallest one, because there is always room for improvement).

Zepto VM is the smallest VM we were able to think about, with an emphasis of using as less RAM as possible. While in theory it might be possible to implement something smaller, in practice it is difficult to go below 1 byte of RAM (which is the minimum overhead by Zepto VM-One).

Note on memory overhead
^^^^^^^^^^^^^^^^^^^^^^^

While Zepto VM itself indeed uses ridiculously low amount of RAM, a developer needs to understand that using some capabilities of Zepto VM will implicitly require more RAM. For example, stacking several replies in one packet will implicitly require more RAM for “reply buffer”. And using “green pseudo-threads” feature will require to store certain portions of the intermediate state of the plugins running simultaneously, at the same time (while without “green pseudo-threads” this RAM can be reused, so the intermediate state of only one plugin needs to be stored at a time).

Zepto VM Restrictions
---------------------

As Zepto VM implements an “Execution Layer” of SACCP, it needs to implement all  “Execution Layer Restrictions” set in :ref:`saccp` document. While present document doesn't duplicate these restrictions, it aims to specify them in appropriate places (for example, when specific instructions are described).

“Program Errors” as specified in Execution Layer Restrictions are implemented as ZEPTOVM_PROGRAMERROR_* Zepto VM exceptions as described below.

Bodyparts and Plugins
---------------------

According to a more general SmartAnthill architecture, each SmartAnthill Device (a.k.a. 'Ant') has one or more sensors and/or actuators, with each sensor or actuator known as an 'ant body part'. Each 'body part' is assigned it's own id, which is stored in 'SmartAnthill Database' within SmartAnthill Client (which in turn is usually implemented by SmartAnthill Central Controller).
For each body part type, there is a 'plugin' (so if there are body parts of the same type in the device, number of plugins can be smaller than number of body parts). Plugins are pieces of code which are written in C language and programmed into MCU of SmartAnthill device.

Bodyparts are identified by BODYPART-ID. BODYPART-IDs MAY be negative. Non-negative values for BODYPART-IDs are used for device-specific body parts. Negative values for BODYPART-IDs are reserved for well-known built-int body parts. Currently, such body parts include:

* **BUILTIN_BODYPART_PAIRING** (see :ref:`sapairing` document for details)
* **BUILTIN_BODYPART_AES** (TODO; MUST NOT allow using any of Device keys, key MUST be provided as a plugin parameter)

Reply Buffer and Reply Frames
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To handle plugins and replies, Zepto VM uses “reply buffer”, which consists of "reply frames". Whenever plugin is called, it is asked to fill its own "reply frame". These "reply frames" are appended to each other in a "reply buffer", so that if there is more than one EXEC instruction, “reply buffer” consists out of "reply frames" in the order of EXEC instructions. As “reply buffer” would be needed regardless of Zepto VM (even simple call to a plugin would need to implement some kind of “reply frame”), it is not considered a part of Zepto VM and it's size is not counted as “memory overhead” of Zepto VM.

Structure of Plugin Data
''''''''''''''''''''''''

Data to be passed to and from plugins is generally described in Plugin Manifest, as described in :ref:`saplugin` document. 

Reply Frame Structure
'''''''''''''''''''''

Reply Frames have the following structure:

**\| OPTIONAL-HEADERS \| FLAGS-AND-SIZE \| REPLY-BODY \|**

where OPTIONAL-HEADERS is described below, FLAGS-AND-SIZE is an Encoded-Unsigned-Int<max=2> field, and REPLY-BODY is data as returned from plugin (possibly truncated, see below), with the size determined by FLAGS-AND-SIZE field as described below.

FLAGS-AND-SIZE field is an Encoded-Unsigned-Int<max=2> bitfield substrate, which is further considered as follows:

* bit [0] is a flag which specifies that there is no more optional headers (always equals 1 when REPLY-DATA immediately follows).
* bit [1] is a flag which specifies if REPLY-DATA has been truncated
* bits [2..] specify size of the REPLY-DATA

OPTIONAL-HEADERS is one or more of optional headers. Each of optional headers has the following structure:

**\| HEADER-FLAGS-AND-SIZE \| HEADER-DATA \|**

where HEADER-FLAGS-AND-SIZE field is an Encoded-Unsigned-Int<max=2> bitfield substrate, which is further considered as follows:

* bit [0] is zero
* bits [1..3] is an type of optional header
* bits [4..] is the size of HEADER-DATA

Optional Headers
''''''''''''''''

Currently, two optional headers are supported: Plugin-Exception optional header, and Plugin-Exception-Call-Trace optional header. 

For Plugin-Exception optional header, type of optional header is 0x0, and HEADER-DATA has the following structure: **\| EXCEPTION-CODE \| EXCEPTION-FILE-HASH \| EXCEPTION-LINE \|**, where EXCEPTION-CODE and EXCEPTION-LINE are Encoded-Unsigned-Int<max=2> fields, and EXCEPTION-FILE-HASH is 2-byte file hash (encoded using "SmartAnthill Endianness").

Plugin-Exception-Call-Trace optional header is optionally added if Plugin-Exception optional header is present, and call trace information is available (in particular, it requires ZEPTO_UNWIND() exception mechanism to be employed).

For Plugin-Exception-Call-Trace optional header, type of optional header is 0x2, and HEADER-DATA is a sequence of **\| TRACE-FILE-HASH \| TRACE-LINE \|** frames (starting from most deep function calls), where TRACE-LINE is an Encoded-Unsigned-Int<max=2> field, and TRACE-FILE-HASH is 2-byte file hash (encoded using "SmartAnthill Endianness").

Plugin-Exception optional header is added if an exception (ZEPTO_THROW, see :ref:`saplugin` document for details) has been thrown while the plugin was executed. 

*NB: due to very limited resources and lack of memory separation support on most MCUs (i.e. all the plugins are effectively running in the same protection ring as the Zepto OS itself), it is very easy to break Zepto OS by injecting an ill-behaved plugin. Zepto OS and Zepto VM are aiming to provide as much debug information as possible, but there are still scenarios when Zepto OS is not able to recover from bugs in plugin, and will not be able to report anything back.*

Packet Chains
-------------

In SACCP (and in Zepto VM as an implementation of SACCP), all interactions between SmartAnthill Client and SmartAnthill Device are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on. Whenever Zepto VM issues a packet to an underlying protocol, it needs to specify whether a packet is a first, intermediate, or last within a “packet chain” (using 'is-first' and 'is-last' flags; note that due to “rules of engagement” described below, 'is-first' and 'is-last' flags are inherently incompatible, which MAY be relied on by implementation). This information allows underlying protocol to arrange for proper retransmission if some packets are lost during communication. See :ref:`saprotostack` document for more details on "packet chains".

Zepto VM Instructions
---------------------

Notation
^^^^^^^^

* Through this document, '\|' denotes field boundaries. All fields (including bitfield substrates, as described in :ref:`saprotostack` document) take a whole number of bytes.
* All Zepto VM instructions have the same basic format: **\| OP-CODE \| OP-PARAMS \|**, where OP-CODE is a 1-byte operation code, and length and content of OP-PARAMS are implicitly defined by OP code.

Zepto VM Opcodes
^^^^^^^^^^^^^^^^

* ZEPTOVM_OP_DEVICECAPS
* ZEPTOVM_OP_EXEC
* ZEPTOVM_OP_PUSHREPLY
* ZEPTOVM_OP_SLEEP
* ZEPTOVM_OP_TRANSMITTER
* ZEPTOVM_OP_MCUSLEEP
* ZEPTOVM_OP_POPREPLIES *\* limited support in Zepto VM-One, full support from Zepto VM-Tiny \*/*
* ZEPTOVM_OP_EXIT
* ZEPTOVM_OP_APPENDTOREPLY *\* limited support in Zepto VM-One, full support from Zepto VM-Tiny \*/*
* */\* starting from the next opcode, instructions are not supported by Zepto VM-One \*/*
* ZEPTOVM_OP_JMP
* ZEPTOVM_OP_JMPIFREPLYFIELD_LT
* ZEPTOVM_OP_JMPIFREPLYFIELD_GT
* ZEPTOVM_OP_JMPIFREPLYFIELD_EQ
* ZEPTOVM_OP_JMPIFREPLYFIELD_NE
* ZEPTOVM_OP_MOVEREPLYTOFRONT
* */\* starting from the next opcode, instructions are not supported by Zepto VM-Tiny and below \*/*
* ZEPTOVM_OP_PUSHEXPR_CONSTANT
* ZEPTOVM_OP_PUSHEXPR_REPLYFIELD
* ZEPTOVM_OP_EXPRUNOP
* ZEPTOVM_OP_EXPRUNOP_EX
* ZEPTOVM_OP_EXPRUNOP_EX2
* ZEPTOVM_OP_EXPRBINOP
* ZEPTOVM_OP_EXPRBINOP_EX
* ZEPTOVM_OP_EXPRBINOP_EX2
* ZEPTOVM_OP_JMPIFEXPR_LT
* ZEPTOVM_OP_JMPIFEXPR_GT
* ZEPTOVM_OP_JMPIFEXPR_EQ
* ZEPTOVM_OP_JMPIFEXPR_NE
* ZEPTOVM_OP_JMPIFEXPR_EX_LT
* ZEPTOVM_OP_JMPIFEXPR_EX_GT
* ZEPTOVM_OP_JMPIFEXPR_EX_EQ
* ZEPTOVM_OP_JMPIFEXPR_EX_NE
* ZEPTOVM_OP_CALL
* ZEPTOVM_OP_RET
* ZEPTOVM_OP_SWITCH
* ZEPTOVM_OP_SWITCH_EX
* ZEPTOVM_OP_INCANDJMPIF
* ZEPTOVM_OP_DECANDJMPIF
* */\* starting from the next opcode, instructions are not supported by Zepto VM-Small and below \*/*
* ZEPTOVM_OP_PARALLEL

Zepto VM Exceptions
-------------------

If Zepto VM encounters a problem, it reports it as an “VM exception” (not to be confused with Plugin-Exception, which is different; normally, on plugin exception Zepto VM records it in respective "reply frame", and continues program execution). Whenever Zepto VM exception characterized by EXCEPTION-CODE occurs, it is processed as follows:

* “reply buffer” is converted into the following format: \|EXCEPTION-CODE\|FLAGS-AND-INSTRUCTION-POSITION\|EXISTING-REPLY-BUFFER-DATA\| , where all fields except for REPLY-BUFFER-DATA, are Encoded-Unsigned-Int<max=2>, and REPLY-BUFFER-DATA fills the rest of the message. In some cases (for example, if there is insufficient RAM), REPLY-BUFFER-DATA MAY be truncated (which is indicated in FLAGS-AND-INSTRUCTION-POSITION field). *Rationale: In certain scenarios, this REPLY-BUFFER-DATA, while incomplete, may allow SmartAnthill Client to extract useful information about the partially successful command.* FLAGS-AND-INSTRUCTION-POSITION field is a 1-byte bitfield substrate, which is further considered as follows:
  
  + bit [0] - specifies if EXISTING-REPLY-BUFFER-DATA has been truncated
  + bits [1..] - specify instruction position where VM exception has occurred

* This reply is passed to the underlying protocol as an 'exception'.

Currently, Zepto VM may issue the following exceptions:

* ZEPTO_VM_INVALIDINSTRUCTION */\* Note that this exception may also be issued when an instruction is encountered which is legal in general, but is not supported by current level of Zepto VM. \*/*
* ZEPTOVM_INVALIDENCODEDSIZE */\* Issued whenever Encoded-\*-Int<max=...> is an invalid encoding, as defined in* :ref:`saprotostack` document *\*/*
* ZEPTOVM_PLUGINERROR
* ZEPTOVM_INVALIDPARAMETER
* ZEPTOVM_INVALIDREPLYNUMBER
* ZEPTOVM_EXPRSTACKUNDERFLOW
* ZEPTOVM_EXPRSTACKINVALIDOFFSET
* ZEPTOVM_EXPRSTACKFROZENVIOLATION
* ZEPTOVM_EXPRSTACKOVERFLOW
* ZEPTOVM_PROGRAMERROR_INVALIDREPLYFLAG
* ZEPTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE

Zepto VM End of Execution
-------------------------

Zepto VM program exits when the sequence of instructions has ended. At this point, an equivalent of **\|EXIT\|<ISLAST>,<0>\|** is implicitly executed (see description of 'EXIT' instruction below); this causes “reply buffer” to be sent back to the SmartAnt Client, with 'is-last' flag set. Alternatively, an “EXIT” instruction (see below) may end program execution explicitly; in this case, parameters to “EXIT” command may specify additional properties as described in "EXIT" instruction description.

Zepto VM Overriding Command
---------------------------

If there is a new command incoming from SmartAnthill Client, while Zepto VM is executing a current program, Zepto VM will (at the very first opportunity) automatically abort execution of the current program, and starts executing the new one. This behaviour is consistent with the concept of “SmartAnthill Client always knows better” which is used throughout the SmartAnthill protocol stack. Such command may be used, for example, by SmartAnthill Client to abort execution of a long-running request and ask SmartAnthill Device to do something else.

Zepto VM Levels
---------------

To accommodate SmartAnthill devices with different capabilities and different amount of RAM, Zepto VM implementations are divided into several levels. Minimal level, which is mandatory for all implementations of Zepto VM, is Level One. Each subsequent Zepto VM level adds support for some new instructions while still supporting all the capabilities of underlying levels.

TODO: timeouts

Level One
^^^^^^^^^

ZeptoVM-One is the absolute minimum implementation of Zepto-VM, which allows to execute only a linear sequence of commands, at the cost of additional RAM needed being 1 byte. ZeptoVM-One supports the following instructions:

**\| ZEPTOVM_OP_DEVICECAPS \| REQUESTED-FIELDS \|**

where ZEPTOVM_OP_DEVICECAPS is 1-byte opcode, and REQUESTED-FIELDS is described below.

DEVICECAPS instruction pushes Device-Capabilities-Reply to "reply buffer" as a "reply frame". Usually, DEVICECAPS instruction is the only instruction in the program. If there are too many requested-fields (for example, they don't fit into RAM, or don't fit into MTU) - as any other "reply frame", it MAY be truncated. 

REQUESTED-FIELDS is a sequence of indicators which configuration parameters are requested:

+------------------------------------------------+-----------------------------+
| Indicator                                      | Return Type                 |
+================================================+=============================+
| SACCP_GUARANTEED_PAYLOAD                       | DEVICE-CAPS-UINT2,          |
|                                                | as described below          |
+------------------------------------------------+-----------------------------+
| ZEPTOVM_LEVEL                                  | 1 byte (enum)               |
+------------------------------------------------+-----------------------------+
| ZEPTOVM_REPLY_BUFFER_AND_EXPR_STACK_BYTE_SIZES | See below                   |
+------------------------------------------------+-----------------------------+
| ZEPTOVM_REPLY_STACK_SIZE                       | DEVICE-CAPS-UINT2           |
+------------------------------------------------+-----------------------------+
| ZEPTOVM_EXPR_FLOAT_TYPE                        | 1 byte (enum)               |
+------------------------------------------------+-----------------------------+
| ZEPTOVM_MAX_PSEUDOTHREADS                      | DEVICE-CAPS-UINT2           |
+------------------------------------------------+-----------------------------+
| DEVICECAPS_END_OF_LIST                         | n/a                         |
+------------------------------------------------+-----------------------------+

DEVICECAPS_END_OF_LIST specifies end of REQUESTED-FIELDS list. Reply to DEVICECAPS instruction contains data which correspond to indicators (and come in the same order as indicators within the request). 

If Device doesn't support specific indicator, it MUST return single byte 0xFF in the appropriate place of the reply buffer. All the valid replies are constructed in the way that cannot possibly have first byte as 0xFF.

DEVICE-CAPS-UINT2 is an Encoded-Unsigned-Int<max=2> which is used as a bitfield substrate, with bit[0] being always 0 (this guarantees that "0xFF is impossible" requirement is met, see about even byte encodings and "indicate an error in an unknown-length field" scenarios in :ref:`saprotostack`), and bits[1..] representing value.

For ZEPTOVM_REPLY_BUFFER_AND_EXPR_STACK_BYTE_SIZES indicator, DEVICECAPS instruction returns 3 fields, first having type DEVICE-CAPS-UINT2, the other two having type Encoded-Unsigned-Int<max=2>. First field is MAX_REPLY_BUFFER_SIZE (in bytes), second field is MAX_EXPR_STACK_BYTE_SIZE (in bytes, so to calculate number of stack entries one needs to use EXPR_FLOAT_TYPE), and third field is MAX_COMBINED_REPLY_BUFFER_SIZE_AND_EXPR_STACK_BYTE_SIZE (in bytes). If device implements "reply buffer" and "expression stack" separately, then device reports MAX_COMBINED_REPLY_BUFFER_SIZE_AND_EXPR_STACK_BYTE_SIZE as equal to MAX_REPLY_BUFFER_SIZE+MAX_EXPR_STACK_BYTE_SIZE; if they use shared portion of memory and one can grow at the expense of another one, then device may report, for example, MAX_REPLY_BUFFER_SIZE = MAX_EXPR_STACK_BYTE_SIZE = MAX_COMBINED_REPLY_BUFFER_SIZE_AND_EXPR_STACK_SIZE.

IMPORTANT: for all the reported sizes, device MUST report them as if implementation does not use any of them for temporary purposes. For example, if implementation has an expression stack with size=8, but when processing a EXPRBINOP_EX2 instruction with PUSH-FLAG=0, implementation first temporarily pushes the result to the top of the stack, before modifying the appropriate value of the stack - such a device MUST report expression stack size reduced by 1 entry (the one used for temporary purposes), i.e. 7*stack_entry_size.


**\| ZEPTOVM_OP_EXEC \| BODYPART-ID \| DATA-SIZE \| DATA \|**

where ZEPTOVM_OP_EXEC is 1-byte opcode, BODYPART-ID is a Encoded-Signed-Int<max=2> id of the bodypart to be used, DATA-SIZE is an Encoded-Unsigned-Int<max=2> (as defined in :ref:`saprotostack` document) length of DATA field, and DATA in an opaque data to be passed to the plugin associated with body part identified by BODYPART-ID; DATA field has size DATA-SIZE.
EXEC instruction invokes a plug-in which corresponds to BODYPART-ID, and passes DATA of DATA-SIZE  size to this plug-in. Plug-in always adds a reply to the reply-buffer; reply size may vary, but MUST be at least 1 byte in length; otherwise it is a ZEPTOVM_PLUGINERROR exception.


**\| ZEPTOVM_OP_PUSHREPLY \| REPLY-BODY-SIZE \| REPLY-BODY \|**

where ZEPTOVM_OP_PUSHREPLY is a 1-byte opcode, REPLY-BODY-SIZE is an Encoded-Unsigned-Int<max=2> (as defined in :ref:`saprotostack` document) size of REPLY-BODY field, and REPLY-BODY is opaque data to be pushed to reply buffer.
PUSHREPLY instruction pushes an additional reply frame with DATA in it to reply buffer.

**\| ZEPTOVM_OP_TRANSMITTER \| ONOFF \|**

where ZEPTOVM_OP_TRANSMITTER is a 1-byte opcode, and ONOFF is a 1-byte field, taking values {0,1}

TRANSMITTER instruction turns transmitter on or off, according to the value of ONOFF field.

**\| ZEPTOVM_OP_SLEEP \| MSEC-DELAY \|**

where ZEPTOVM_OP_SLEEP is a 1-byte opcode, and MSEC-DELAY is an Encoded-Unsigned-Int<max=4> field (as defined in :ref:`saprotostack` document).
Pauses execution for approximately MSEC-DELAY milliseconds. Exact delay times are not guaranteed; specifically, SLEEP instruction MAY take significantly longer than requested.

**\| ZEPTOVM_OP_MCUSLEEP \| SEC-DELAY \| TRANSMITTERONWHENBACK-AND-MAYDROPEARLIERINSTRUCTIONS \|**

where ZEPTOVM_OP_MCUSLEEP is a 1-byte opcode, SEC-DELAY is an Encoded-Unsigned-Int<max=4> field (as defined in :ref:`saprotostack` document), and TRANSMITTERONWHENBACK-AND-MAYDROPEARLIERINSTRUCTIONS is a 1-byte bitfield substrate, with bit [0] being TRANSMITTERONWHENBACK, bit [1] being MAYDROPEARLIERINSTRUCTIONS, and bits [2..7] being reserved (MUST be zero).

MCUSLEEP instruction puts MCU into sleep-with-timer mode for approximately SEC-DELAY seconds. If sleep-with-timer mode is not available with current MCU, then such an instruction still may be sent to such a device, as a means of long delay, and SmartAnthill device MUST process it just by waiting for specified time. TRANSMITTERONWHENBACK specifies if device transmitter should be turned on after MCUSLEEP, and MAYDROPEARLIERINSTRUCTIONS is an optimization flag which specifies if MCUSLEEP is allowed to drop the portion of the ZeptoVM program which is located before MCUSLEEP, when going to sleep (this may allow to provide certain savings, see below).

As MCUSLEEP may disable device receiver, Zepto VM enforces relevant “Execution Layer Restrictions” when MCUSLEEP is invoked; to ensure consistent behavior between MCUs, these restriction MUST be enforced regardless of MCUSLEEP really disabling device receiver. Therefore (NB: these checks SHOULD be implemented for ZeptoVM-One; they MUST be implemented for all Zepto-VM levels other than ZeptoVM-One):

* If original command has not had an ISLAST flag, and MCUSLEEP is invoked, it causes a ZEPTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE exception.
* Zepto VM keeps track if MCUSLEEP was invoked; this 'mcusleep-invoked' flag is used by some other instructions.
* NB: calling MCUSLEEP twice within the same program is allowed, so if 'mcusleep-invoked' flag is already set and MCUSLEEP is invoked, this is not a problem

It should be noted that implementing MCUSLEEP instruction will implicitly require storing current program, current PC and current “reply buffer” either in EEPROM, or to request MPU to preserve RAM while waiting. This will be done automagically by Zepto VM, but it is not without it's cost. It might be useful to know that in some cases this cost is lower when amount of data to be preserved is small (for example, it happens when “reply buffer” is empty, and/or when MAYDROPEARLIERINSTRUCTIONS is used and the remaining program is small).

**\| ZEPTOVM_OP_POPREPLIES \| N-REPLIES \|**

where ZEPTOVM_OP_POPREPLIES is a 1-byte opcode (NB: it is the same as ZEPTOVM_OP_POPREPLIES in Level Tiny), and N-REPLIES is an Encoded-Unsigned-Int<max=2> field, which MUST be 0 for Zepto VM-One (other values are allowed for Zepto VM-Tiny and above, as described below). If N-REPLIES is not 0 for Zepto VM-One POPREPLIES instruction, Zepto VM will issue a ZEPTOVM_INVALIDPARAMETER exception. \|POPREPLIES\|0\| means “remove all replies currently in reply buffer”.

NB: Zepto VM-One implements POPREPLIES instruction only partially (for N-REPLIES=0); Zepto VM-Tiny and above support other values as described below, and behavior for N-REPLIES=0 which is supported by all Zepto VM levels, is exactly the same regardless of level.

**\| ZEPTOVM_OP_EXIT \| REPLY-FLAGS-AND-FORCED-PADDING-FLAG \| (opt) FORCED-PADDING-TO \|**

where ZEPTOVM_OP_EXIT is a 1-byte opcode (NB: it is the same as ZEPTOVM_OP_EXIT in Level Tiny), REPLY-FLAGS-AND-FORCED-PADDING-FLAG is a 1-byte bitfield substrate, with bits[0..1] being REPLY-FLAGS bitfield, taking one of the following values: {NONE,ISFIRST,ISLAST}, bit [2] being FORCED-PADDING-FLAG bitfield which stores {0,1}, bits [3..7] being reserved (MUST be zero), and FORCED-PADDING-TO is an Encoded-Unsigned-Int<max=2> (as defined in :ref:`saprotostack` document) field, which is present only if <FORCED-PADDING-FLAG> is equal to 1.

EXIT instruction posts all the replies which are currently in the “reply buffer”, back to SmartAnthill Central Controller, and terminates the program. Device receiver is kept turned on after the program exits (so the device is able to accept new commands).

To enforce “Execution Layer Requirements”, the following SHOULD be enforced for Zepto VM-One and MUST be enforced for other Zepto VM layers:

* if 'mcusleep-invoked' flag is not set, and original command has had ISLAST flag, then “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS != ISFIRST (this is an usual command-reply pattern)
* if 'mcusleep-invoked' flag is not set, and original command has not had ISLAST flag, then “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS == ISFIRST (this is a 'long command-reply' pattern)
* if 'mcusleep-invoked' flag is set, then original command will have ISLAST flag (because of other restrictions; this means violating 'ISLAST' requirement while processing EXIT instruction is not an exception, but an internal assertion which MUST NOT happen); “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS == ISFIRST (this is a 'mcusleep-then-wake' pattern)

If any of the restrictions above is not compied with, Zepto VM generates a ZEPTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE exception.

FORCED-PADDING-TO field (if present) specifies 'enforced padding' as described in :ref:`sascrambling` document. Essentially:

* if present, FORCED-PADDING-TO MUST specify length which is equal to or greater than the size of current "reply buffer"
* if developer wants to avoid information leak from the fact that encrypted messages may have different lengths, she may specify the same FORCED-PADDING-TO for all the replies which should be indistinguishable.

**\| ZEPTOVM_OP_APPENDTOREPLY \| REPLY-NUMBER \| DATA-TYPE \| DATA \|**

where REPLY-NUMBER is a Encoded-Signed-Int<max=2> field, which MUST be equal to '-1' for Zepto VM-One, DATA-TYPE is 1-byte taking one of the valid values for FIELD-SEQUENCE field as described in JMPIFREPLYFIELD instruction, and DATA has size determined by DATA-TYPE field.

NB: Zepto VM-One implements APPENDTOREPLY instruction only partially (for REPLY-NUMBER=-1); Zepto VM-Tiny and above support other values as described below, and behavior for REPLY-NUMBER=-1 which is supported by all Zepto VM levels, is exactly the same regardless of level.

Implementation notes
''''''''''''''''''''

If strict checks of “Execution Layer Restrictions” are disabled (which is allowed only for Zepto VM-One and not for any other level), then only PC (Program Counter) needs to be maintained for operating Level One.

To keep track of “Execution Layer Restrictions”, a one-byte flag bitmask is used with the following flags:

* mcusleep-invoked
* *currently there are no other flags*

Memory overhead
'''''''''''''''

Memory overhead of ZeptoVM-One is 1 byte; if “Execution Layer Restrictions” are strictly enforced (which is a MUST for all levels except for Zepto VM-One), this requires an additional 1 byte.

Level Tiny
^^^^^^^^^^

Zepto VM-Tiny allows for more complicated programs, including basic conditions, at the cost of additional memory needed being on the order of 5-10 bytes. Zepto VM-Tiny, in addition to instructions supported by Zepto VM-One, additionally supports the following instructions:

**\| ZEPTOVM_OP_JMP \| DELTA \|**

where ZEPTOVM_OP_JMP is a 1-byte opcode, and DELTA is an Encoded-Signed-Int<max=2> signed integer which denotes how PC (program counter) should be changed (DELTA is always considered in relation to the *end* of current instruction, so JMP 0 is effectively a no-op).

**\| ZEPTOVM_OP_JMPIFREPLYFIELD_<SUBCODE> \| REPLY-NUMBER \| FIELD-SEQUENCE \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ}; ZEPTOVM_OP_JMPIFREPLYFIELD_LT, ZEPTOVM_OP_JMPIFREPLYFIELD_GT, and ZEPTOVM_OP_JMPIFREPLYFIELD_EQ are 1-byte opcodes, REPLY-NUMBER is an Encoded-Signed-Int<max=2>, FIELD-SEQUENCE is described below, THRESHOLD is an Encoded-Signed-Int<max=2> field, and interpretation of DELTA is similar to that of in JMP instruction description.

REPLY-NUMBER is a number of reply frame in "reply buffer". Negative values mean 'from the end of buffer', so that REPLY-NUMBER=-1 means 'last reply in reply buffer'. If REPLY-NUMBER points to a non-existing item in "reply buffer" (that is, it is positive and is >= number-of-replies, or it is negative and is <= -number-of-replies TODO:check), it is a ZEPTOVM_INVALIDREPLYNUMBER exception.

FIELD-SEQUENCE field describes a sequence of fields to be read from plugin reply body (that is, after all the optional headers, flags etc. are processed); normally, for SmartAnthill systems, it is derived from SmartAnthill Plugin Manifest during program preparation. Last field in FIELD-SEQUENCE always represents a field to be read; all previous fields are skipped. FIELD-SEQUENCE is encoded as a byte sequence with the following byte values supported:

* ENCODED_UNSIGNED_INT_FIELD
* ENCODED_SIGNED_INT_FIELD
* ONE_BYTE_FIELD
* TWO_BYTE_FIELD (assumes 'SmartAnthill endianness' as described in :ref:`saprotostack` document)
* HALF_FLOAT_FIELD (using encoding as described in :ref:`saprotostack` document for half-floats)
* END_OF_SEQUENCE

ZEPTOVM_OP_JMPIFREPLYFIELD_* instruction takes the reply of the last plugin which was called, and compares required field to the THRESHOLD. If first byte of the reply is < (for <SUBCODE>=LT) THRESHOLD, PC is incremented by a value of DELTA (as with JMP, DELTA is added to a PC positioned right after current instruction).

+-----------+--------------------+
| <SUBCODE> | Jump if            |
+===========+====================+
| LT        | Field < THRESHOLD  |
+-----------+--------------------+
| GT        | Field > THRESHOLD  |
+-----------+--------------------+
| EQ        | Field == THRESHOLD |
+-----------+--------------------+
| NE        | Field != THRESHOLD |
+-----------+--------------------+

**\| ZEPTOVM_OP_POPREPLIES \| N-REPLIES \|**

where ZEPTOVM_OP_POPREPLIES is a 1-byte opcode and N-REPLIES is an Encoded-Unsigned-Int<max=2> field representing number of replies to be popped.

POPREPLIES instruction removes last N-REPLIES of plugins from the reply buffer. If N-REPLIES is equal to zero, it means that all replies are removed. If N-REPLIES is more than number of replies in the buffer, it is a TODO exception. Usually, either \|POPREPLIES\|0\| (removing all the replies) or \|POPREPLIES\|1\| (removing only one reply) is used, but other values are also possible.

NB: POPREPLIES instruction is partially implemented by Zepto VM-One (for N-REPLIES=0).

**\| ZEPTOVM_OP_MOVEREPLYTOFRONT \| REPLY-NUMBER \|**

where ZEPTOVM_OP_MOVEREPLYTOFRONT is a 1-byte opcode and REPLY-NUMBER is an Encoded-Signed-Int<max=2> field, which is interpreted as described in JMPIFREPLYFIELD instruction.

MOVEREPLYTOFRONT instruction is used to reorder reply frames within reply buffer. It takes reply frame which has REPLY-NUMBER, and makes it the first one in the buffer, moving the rest of the replies back. Implementation note: need also to recalculate and update positions in offset stack.

**\| ZEPTOVM_OP_APPENDTOREPLY \| REPLY-NUMBER \| DATA-TYPE \| DATA \|**

where REPLY-NUMBER is interpreted as described in JMPIFREPLYFIELD instruction, DATA-TYPE is 1-byte taking one of the valid values for FIELD-SEQUENCE field as described in JMPIFREPLYFIELD instruction, and DATA has size determined by DATA-TYPE field.

APPENDTOREPLY instruction appends DATA with DATA-TYPE to the end of reply specified by REPLY-NUMBER.

NB: APPENDTOREPLY instruction is partially implemented by Zepto VM-One (for REPLY-NUMBER=-1).

Implementation notes
''''''''''''''''''''

To implement Zepto VM-Tiny, in addition to PC required by Zepto VM-One, a stack of offsets which signify positions of reply frames in “reply buffer”, needs to be maintained. Such stack should consist of an array of bytes for offsets, and additional byte to store number of entries on the stack. Size of this stack is a ZEPTOVM_REPLY_STACK_SIZE parameter of Zepto VM-Tiny (which is stored in SmartAnthill DB on SmartAnthill Client and reported via DEVICECAPS instruction).

Memory overhead
'''''''''''''''

Memory overhead of ZeptoVM-Tiny is (in addition to overhead of ZeptoVM-One) is 1+ZEPTOVM_REPLY_STACK_SIZE (or 1+2*ZEPTOVM_REPLY_STACK_SIZE if size of reply buffer can be over 256 bytes).

Level Small
^^^^^^^^^^^

Zepto VM-Small allows for even more complicated programs, including expressions and loops, at the cost of additional memory needed (in addition to Zepto VM-Tiny) being on the order of 9-65 (depending on complexity of programs to be supported) bytes.
Zepto VM-Small, in addition to instructions supported by Zepto VM-Tiny, additionally supports the following instructions:

**\| ZEPTOVM_OP_PUSHEXPR_CONSTANT \| CONST \|**

where ZEPTOVM_OP_PUSHEXPR_CONSTANT is 1-byte opcode, and CONST is a 2-byte half-float constant (encoded as described in :ref:`saprotostack`) to be pushed to expression stack.

PUSHEXPR_CONSTANT instruction pushes CONST to an expression stack (if expression stack is exceeded, it will cause ZEPTOVM_EXPRSTACKOVERFLOW VM exception).

**\| ZEPTOVM_OP_PUSHEXPR_REPLYFIELD \| REPLY-NUMBER \| FIELD-SEQUENCE \|**

ZEPTOVM_OP_PUSHEXPR_REPLYFIELD is 1-byte opcode, REPLY-NUMBER and FIELD-SEQUENCE are similar to that of in JMPIFREPLYFIELD instruction. 

PUSHEXPR_REPLYFIELD takes a field (specified by FIELD-SEQUENCE) from reply frame (specified by REPLY-NUMBER), and pushes it to the expression stack (if expression stack is exceeded, it will cause ZEPTOVM_EXPRSTACKOVERFLOW VM exception). If data in the field doesn't fit into stack type (see below), it is an ZEPTOVM_INVALIDEXPRDATA exception. 

**\| ZEPTOVM_OP_EXPRUNOP \| UNOP \|**

where ZEPTOVM_OP_EXPRUNOP is a 1-byte opcode, and UNOP is 1-byte taking one of the following values:

+-----------+-------------------------------+
|UNOP       |Corresponding unary C operation|
+===========+===============================+
|UNOP_POP   | N/A                           |
+-----------+-------------------------------+
|UNOP_COPY  | \=                            |
+-----------+-------------------------------+
|UNOP_MINUS | \-                            |
+-----------+-------------------------------+
|UNOP_BITNEG| ~                             |
+-----------+-------------------------------+
|UNOP_NOT   | !                             |
+-----------+-------------------------------+
|UNOP_INC   | +1                            |
+-----------+-------------------------------+
|UNOP_DEC   | -1                            |
+-----------+-------------------------------+

EXPRUNOP instruction pops topmost value from the expression stack, modifies it according to the table above, and pushes modified value back to expression stack. All operations are performed as specified in the table above; '-', '+1' and '-1' operations are performed as floating-point operation (see details below), for '~' and '!' operations the operand is first converted into integer with zero exponent (and then only fraction is involved in these operations). If expression stack is empty, it will cause a ZEPTOVM_EXPRSTACKUNDERFLOW VM exception. Overflows are handled in a normal manner for floats (NB: as it is float arithmetics, '+1' and '-1' operations MAY cause operand to stay without changes even if no 'infinity' has occurred; it means that if half-floats are used as expression stack values, 2048+1 results in 2048, causing potential for infinite loops TODO: check if it is 2048 or 2050).

If UNOP is UNOP_POP, then no value is pushed back to the expression stack (i.e. UNOP_POP causes one value to be removed from the expression stack).

**\| ZEPTOVM_OP_EXPRUNOP_EX \| UNOP \| POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OPERAND \|**

where ZEPTOVM_OP_EXPRUNOP_EX is a 1-byte opcode, UNOP is similar to that of in EXPRUNOP instruction, POP-FLAG-AND-EXPR-OFFSET is an Encoded-Signed-Int<max=2> field, which acts as a substrate for POP-FLAG bitfield (occupies bit [0]), and EXPR-OFFSET bitfield (occupies bits [1..]), and OPTIONAL-IMMEDIATE-OPERAND is a half-float field, present only if EXPR-OFFSET is zero (see also below). EXPR-OFFSET specifies expression index which is used by EXPRUNOP_EX instruction, as follows: zero value means that the operand is an immediate operand, positie values of EXPR-OFFSET mean "values from top of the expression stack", so '1' means 'topmost value on the stack'; negative values mean 'values from beginning of the stack', so that '-1' means expr_stack[0], '-2' means expr_stack[1] and so on (negative values of EXPR-OFFSET can be used, for example, to simulate global variables). POP-FLAG specifies whether the slot specified by EXPR-OFFSET is removed from the expression stack after the calculation is performed (if it is not topmost value which is popped, it causes collapsing the stack as necessary). Accordingly, **\| EXPRUNOP_EX \| UNOP \| POP-FLAG=1, EXPR-OFFSET=1 \|** is equivalent to **\| EXPRUNOP \| UNOP \|**. If EXPR-OFFSET points beyond the current size of expression stack, this will cause a ZEPTOVM_EXPRSTACKINVALIDOFFSET exception. If POP-FLAG is 1 and popping would lead to the modification of "frozen" part of the expression stack (see description of "frozen" stack in the context of PARALLEL instruction), it will cause a ZEPTOVM_EXPRSTACKFROZENVIOLATION exception. If POP-FLAG is 1 and EXPR-OFFSET is zero (which would mean 'pop immediate operand'), it is ZEPTOVM_INVALIDPARAMETER exception.

EXPRUNOP_EX instruction is similar to EXPRUNOP instruction, but allows to use wider range of the operand (with popping from the stack being optional).

**\| ZEPTOVM_OP_EXPRUNOP_EX2 \| UNOP \| POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OPERAND \| PUSH-FLAG-AND-PUSH-EXPR-OFFSET \|**

where ZEPTOVM_OP_EXPRUNOP_EX2 is a 1-byte opcode, UNOP, POP-FLAG-AND-EXPR-OFFSET, and OPTIONAL-IMMEDIATE-OPERAND are  similar to that of in EXPRUNOP_EX instruction, PUSH-FLAG-AND-EXPR-OFFSET is an Encoded-Signed-Int<max=2> field, which acts as a substrate for PUSH-FLAG bitfield (occupies bit [0]), and PUSH-EXPR-OFFSET bitfield (occupies bits [1..]). PUSH-EXPR-OFFSET specifies target expression index which is used by EXPRUNOP_EX2 instruction, as follows: 

* PUSH-EXPR-OFFSET=0 means that result should be pushed on top of expression stack; PUSH-FLAG MUST be =1 in this case (otherwise it is ZEPTOVM_INVALIDPARAMETER exception). 
* PUSH-FLAG=0 and PUSH-EXPR-OFFSET != 0 means that a value on expression stack (specified by PUSH-EXPR-OFFSET, which is treated similar to EXPR-OFFSET; in particular, both negative and positive values of PUSH-EXPR-OFFSET are valid) needs to be replaced with a result of calculation
* PUSH-FLAG=1 and PUSH-EXPR-OFFSET != 0 means that a calculated value needs to be inserted within expression stack; index of the stack *before* which index such insertion needs to be made, is specified by PUSH-EXPR-OFFSET (in a manner similar to EXPR-OFFSET; in particular, both negative and positive values of PUSH-EXPR-OFFSET are valid; for example, PUSH-EXPR-OFFSET=-1 means that new value needs to be inserted into the very beginning of the stack, and PUSH-EXPR-OFFSET=1 means that new value should be inserted right before the topmost value - so it will become second-topmost after insertion).

EXPRUNOP_EX2 instruction is similar to EXPRUNOP_EX instruction, but allows to specify where the result of calculation needs to be placed. **\| EXPRUNOP_EX2 \| UNOP \| POP-FLAG=1,EXPR-OFFSET=1 \| PUSH-FLAG=1, PUSH-EXPR-OFFSET=0 \|** is equivalent to **\| EXPRUNOP \| UNOP \|**.

**\| ZEPTOVM_OP_EXPRBINOP \| BINOP \|**

where ZEPTOVM_OP_EXPRBINOP is a 1-byte opcode, and BINOP is 1-byte taking one of the following values:

+------------+--------------------------------+
|BINOP       |Corresponding binary C operation|
+============+================================+
|BINOP_PLUS  | \+                             |
+------------+--------------------------------+
|BINOP_MINUS | \-                             |
+------------+--------------------------------+
|BINOP_SHL   | <<                             |
+------------+--------------------------------+
|BINOP_SHR   | >>                             |
+------------+--------------------------------+
|BINOP_USHR  | Java-like >>>                  |
+------------+--------------------------------+
|BINOP_BITAND| &                              |
+------------+--------------------------------+
|BINOP_BITOR | \|                             |
+------------+--------------------------------+
|BINOP_AND   | &&                             |
+------------+--------------------------------+
|BINOP_OR    | ||                             |
+------------+--------------------------------+

EXPRBINOP instruction pops two topmost values from the expression stack, calculates result out of them according to the table above (as 'second topmost' op 'topmost'), and pushes calculated value back to the expression stack. All operations are performed as specified in the table above; '+' and '-' are performed as floating-point operations (see details below), for '<<', '>>', '&', '|', '&&', and '||' both operands are first converted into integers with zero exponent (and then only significands of operands are involved in these operations). If expression stack has less than two items, it will cause a ZEPTOVM_EXPRSTACKUNDERFLOW VM exception. Overflows are handled in a standard manner for floats (causing 'infinity' result when necessary). NB: there are no multiplication/division operations for Zepto VM-Small, they're introduced in higher Zepto-VM levels.

**\| ZEPTOVM_OP_EXPRBINOP_EX \| BINOP \| OP1-POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OP1 \| OP2-POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OP2 \|**

where ZEPTOVM_OP_EXPRBINOP_EX is a 1-byte opcode, BINOP is 1-byte taking the same values as for ZEPTOVM_OP_EXPRBINOP, OP1-POP-FLAG-AND-EXPR-OFFSET and OP2-POP-FLAG-AND-EXPR-OFFSET are similar to POP-FLAG-AND-EXPR-OFFSET in EXPRUNOP_EX instruction, OPTIONAL-IMMEDIATE-OP1 is a half-float field present only if EXPR-OFFSET within OP1-POP-FLAG-AND-EXPR-OFFSET is zero, and OPTIONAL-IMMEDIATE-OP2 is a half-float field present only if EXPR-OFFSET within OP2-POP-FLAG-AND-EXPR-OFFSET is zero. 

EXPRBINOP_EX instruction is similar to EXPRBINOP instruction, but allows to use wider range of operands (with popping from the stack being optional). **\| EXPRBINOP_EX \| BINOP \| POP-FLAG=1,EXPR-OFFSET=2 \| POP-FLAG=1,EXPR-OFFSET=1 \|** is equivalent to **\| EXPRBINOP \| BINOP \|**.

**\| ZEPTOVM_OP_EXPRBINOP_EX2 \| BINOP \| OP1-POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OP1 \| OP2-POP-FLAG-AND-EXPR-OFFSET \| OPTIONAL-IMMEDIATE-OP2 \| PUSH-FLAG-AND-PUSH-EXPR-OFFSET \|**

where ZEPTOVM_OP_EXPRBINOP_EX2 is a 1-byte opcode, BINOP, OP*-POP-FLAG-AND-EXPR-OFFSET and OPTIONAL-IMMEDIATE-OP* fields are similar to that of in EXPRBINOP_EX instruction, and PUSH-FLAG-AND-PUSH-EXPR-OFFSET is similar to that of in EXPRUNOP_EX2 instruction.

EXPRBINOP_EX2 instruction is similar to EXPRBINOP_EX instruction, but allows to to specify where the result of calculation needs to be placed. **\| EXPRBINOP_EX2 \| BINOP \| POP-FLAG=1,EXPR-OFFSET=2 \| POP-FLAG=1,EXPR-OFFSET=1 \| PUSH-FLAG=1, PUSH-EXPR-OFFSET=0 \|** is equivalent to **\| EXPRBINOP \| BINOP \|**.

**\| ZEPTOVM_OP_JMPIFEXPR <SUBCODE> \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ,NE}; ZEPTOVM_OP_JMPIFEXPR_LT, ZEPTOVM_OP_JMPIFEXPR_GT, ZEPTOVM_OP_JMPIFEXPR_EQ, and  ZEPTOVM_OP_JMPIFEXPR_NE are 1-byte opcodes, THRESHOLD is a 2-byte half-float constant (encoded as described in :ref:`saprotostack`), and interpretation of DELTA is similar to that of in JMP description.

+---------+----------------------------------------------------+
|<SUBCODE>|Jump if                                             |
+=========+====================================================+
|LT       | Topmost value on the expression stack < THRESHOLD  |
+---------+----------------------------------------------------+
|GT       | Topmost value on the expression stack > THRESHOLD  |
+---------+----------------------------------------------------+
|EQ       | Topmost value on the expression stack == THRESHOLD |
+---------+----------------------------------------------------+
|NE       | Topmost value on the expression stack != THRESHOLD |
+---------+----------------------------------------------------+

JMPIFEXPR <SUBCODE> instruction pops the topmost value from the expression stack, compares it with THRESHOLD according to <SUBCODE>, and updates Program Counter by DELTA if condition specified by comparison is met (as with JMP, DELTA is added to a PC positioned right after current instruction). If expression stack is empty, it will cause a ZEPTOVM_EXPRSTACKUNDERFLOW VM exception.

TODO: can equivalents for LE/GE be strictly derived in case of floats?

**\| ZEPTOVM_OP_JMPIFEXPR_EX <SUBCODE> \| POP-FLAG-AND-EXPR-OFFSET \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ,NE}; ZEPTOVM_OP_JMPIFEXPR_EX_LT, ZEPTOVM_OP_JMPIFEXPR_EX_GT, ZEPTOVM_OP_JMPIFEXPR_EX_EQ, and  ZEPTOVM_OP_JMPIFEXPR_EX_NE are 1-byte opcodes, POP-FLAG-AND-EXPR-OFFSET is treated similar to that of in EXPRUNOP_EX instruction, THRESHOLD is a 2-byte half-float constant (encoded as described in :ref:`saprotostack`), and interpretation of DELTA is similar to that of in JMP description.

JMPIFEXPR_EX <SUBCODE> instruction works similar to JMPIFEXPR instruction, but allows for a wider range of the operand (with popping from the stack being optional).

**\| ZEPTOVM_OP_CALL \| PROC-ADDR \|**

where ZEPTOVM_OP_CALL is a 1-byte opcode, and PROC-ADDR is an Encoded-Unsigned-Int<max=2> which specifies an absolute address of the procedure/function being called.

CALL instruction pushes current value of the Program Counter (PC) to the top of expression stack, and sets Program Counter to PROC-ADDR. 

Passing parameters to a procedure/function is a matter of convention between caller and callee. For example, caller may  push values to the top of the expression stack, and callee may read them then; cleaning parameters from the stack may be performed by caller after callee returns. 

*IMPORTANT: the way of the address being pushed to expression stack is not specified, and therefore it is prohibited to use RET instruction to implement calculated jumps. Debug versions of Zepto VM SHOULD implement an additional bit array specifying which expression stack entries are CALL entries, and raise a TBD exception whenever CALL stack entry is used for calculations, and whenever non-CALL stack entry is used for RET*

**\| ZEPTOVM_OP_RET \|**

where ZEPTOVM_OP_RET is a 1-byte opcode. 

RET instruction pops value of the Program Counter (PC) from the expression stack (the one which has been previously pushed by CALL instruction), effectively performing return from the procedure/function.

**\| ZEPTOVM_OP_SWITCH \| NUMBER-OF-ENTRIES \| SWITCH-ENTRY \| ... \| SWITCH-ENTRY \|**

where ZEPTOVM_OP_SWITCH is a 1-byte opcode, NUMBER-OF-ENTRIES is an Encoded-Unsigned-Int<max=2> field, which specifies number of SWITCH-ENTRY's. 

Each of SWITCH-ENTRY's has a format of **\| CASE-VALUE \| DELTA \|**, where CASE-VALUE is an Encoded-Signed-Int<max=depends-on-stack-expr-type> field, and DELTA is treated similar to that of in JMP instruction (as usual, DELTA is applied to the *end* of current instruction, i.e. to the end of SWITCH instruction). 

SWITCH instruction pops a topmost value from the expression stack, converts it to the integer, and then compares it to CASE-VALUE's of each SWITCH-ENTRY, and performs jump if the (converted) value from expression stack matches an appropriate CASE-VALUE. If none of CASE-VALUE's matches, then execution continues after SWITCH instruction.

**\| ZEPTOVM_OP_SWITCH_EX \| POP-FLAG-AND-EXPR-OFFSET \| NUMBER-OF-ENTRIES \| SWITCH-ENTRY \| ... \| SWITCH-ENTRY \|**

where ZEPTOVM_OP_SWITCH_EX is a 1-byte opcode, POP-FLAG-AND-EXPR-OFFSET is treated similar to that of EXPRUNOP_EX instruction, and NUMBER-OF-ENTRIES and SWITCH-ENTRIES are similar to that of SWITCH instruction.

SWITCH_EX instruction works similar to SWITCH instruction, but allows for a wider range of the operand (with popping from the stack being optional).

**\| ZEPTOVM_OP_INCANDJMPIF \| EXPR-OFFSET \| THRESHOLD \| DELTA \|**

where ZEPTOVM_OP_INCANDJMPIF is a 1-byte opcode, EXPR-OFFSET is an Encoded-Signed-Int<max=2> (treated similar to EXPR-OFFSET bitfield in EXPRUNOP instruction), THRESHOLD is similar to that of in JMPIFEXPR instruction, and DELTA is similar to that of in JMP instruction. 

**\| INCANDJMPIF \| EXPR-OFFSET \| THRESHOLD \| DELTA \|** instruction is a shortcut for **\| EXPRUNOP_EX \| UNOP_INC \| POP-FLAG=0,EXPR-OFFSET=EXPR-OFFSET \| JMPIFEXPR_EX LT \| POP-FLAG=0,EXPR-OFFSET=EXPR-OFFSET \| THRESHOLD \| DELTA \|**. It can be used, for example, at the end of the `for(int i=0; i < 5; i++) {...}` loop (use within while and do-while loops is similar).


**\| ZEPTOVM_OP_DECANDJMPIF \| EXPR-OFFSET \| THRESHOLD \| DELTA \|**

where ZEPTOVM_OP_DECANDJMPIF is a 1-byte opcode, EXPR-OFFSET is an Encoded-Signed-Int<max=2> (treated similar to EXPR-OFFSET bitfield in EXPRUNOP instruction), and DELTA is similar to that of in JMP instruction. 

**\| DECANDJMPIF \| EXPR-OFFSET \| THRESHOLD \| DELTA \|** instruction is a shortcut for **\| EXPRUNOP_EX \| UNOP_DEC \| POP-FLAG=0,EXPR-OFFSET=EXPR-OFFSET \| JMPIFEXPR_EX GT \| POP-FLAG=0,EXPR-OFFSET=EXPR-OFFSET \| THRESHOLD \| DELTA \|**. It can be used, for example, at the end of the `for(int i=5; i > 0; i--) {...}` loop (use within while and do-while loops is similar).


Implementation notes
''''''''''''''''''''

To implement Zepto VM-Small, in addition to PC and reply-offset-stack required by Zepto VM-Tiny, an expression stack of floating-point values, need to be maintained. Such stack should consist of an array of floating-point values, and an additional byte to store number of entries on the stack. Size of this stack is a ZEPTOVM_EXPR_STACK_SIZE parameter of Zepto VM-Small (which is stored in SmartAnthill DB on SmartAnthill Client and reported via DEVICECAPS instruction). 

*Type of the values on expression stack always has floating point semantics*, and is one of the following: ROUGH_HALF_FLOAT (2 bytes; same as HALF_FLOAT, but with reduced calculation precision - TBD), HALF_FLOAT (2-byte float, see http://en.wikipedia.org/wiki/Half-precision_floating-point_format), FLOAT (4-byte float), DOUBLE (8-byte float); one of these constants is returned in DEVICECAPS instruction reply to indicate kind of floating point arithmetics supported by specific device; each subsequent floating point format is an extension over previous one. 

Memory overhead
'''''''''''''''

Memory overhead of ZeptoVM-Small is (in addition to overhead of ZeptoVM-Tiny) is 1+stack_entry_size*ZEPTOVM_EXPR_STACK_SIZE.

Turing Completeness
'''''''''''''''''''

Starting from Zepto VM-Small, Zepto VM implementations are techically Turing complete. TODO: check

Zepto VM-Medium
^^^^^^^^^^^^^^^

Zepto VM-Medium adds support for parallel execution, and TODO: remote debugging.

**\| ZEPTOVM_OP_PARALLEL \| N-PSEUDO-THREADS \| PSEUDO-THREAD-1-INSTRUCTIONS-SIZE \| PSEUDO-THREAD-1-INSTRUCTIONS \| ... \| PSEUDO-THREAD-N-INSTRUCTIONS-SIZE \| PSEUDO-THREAD-N-INSTRUCTIONS \|**

where ZEPTOVM_OP_PARALLEL is 1-byte opcode, N-PSEUDO-THREADS is a number of "pseudo-threads" requested, 'PSEUDO-THREAD-X-INSTRUCTIONS-SIZE' is Encoded-Unsigned-Int<max=2> (as defined in :ref:`saprotostack` document) size of PSEUDO-THREAD-X-INSTRUCTIONS, and PSEUDO-THREAD-X-INSTRUCTIONS is a sequence of Zepto VM commands which belong to the pseudo-thread #X. Within PSEUDO-THREAD-X-INSTRUCTIONS, all commands of Zepto VM are allowed, with an exception of PARALLEL, EXIT and any jump instruction which leads outside of the current pseudo-thread.

PARALLEL instruction starts processing of several pseudo-threads. PARALLEL instruction is considered completed when all the pseudo-threads reach the end of their respective instructions. Normally, it is implemented via state machines (see :ref:`sazeptoos` document for details), so it is functionally equivalent to "green threads" (and not to "native threads").

When PARALLEL instruction execution is started, original "reply buffer" is "frozen" and cannot be modified by any of the pseudo-threads; each pseudo-thread has it's own "reply buffer" which is empty at the beginning of the pseudo-thread execution. After PARALLEL instruction is completed (i.e. all pseudo-threads have been terminated), the original "reply buffer" which existed before PARALLEL instruction has started, is restored, and all the pseudo-thread "reply buffers" which existed right before after respective pseudo-threads are terminated, are added to the end of the original "reply buffer"; this allows to have instructions such as EXEC and PUSHREPLY within the pseudo-threads; this adding of pseudo-thread "reply buffers" to the end of original "reply buffer" always happens in order of pseudo-thread descriptions within the PARALLEL instruction (and is therefore does *not* depend on the race conditions between different pseudo-threads).

When PARALLEL instruction execution is started, original expression stack is "frozen" and cannot be modified by any of the pseudo-threads (though it may be read using EXPR*_EX instructions as described below); each pseudo-thread has it's own expression stack which is empty at the beginning of the pseudo-thread execution. After PARALLEL instruction is completed (i.e. all pseudo-threads have been terminated), the original expression stack which existed before PARALLEL instruction has started, is restored, and all the pseudo-thread expression stacks remaining after respective pseudo-threads are terminated, are added to the top of this original stack; this allows to easily pass information from pseudo-threads to the main program; this adding of pseudo-thread expression stacks on top of original expression stack always happens in order of pseudo-thread descriptions within the PARALLEL instruction (and is therefore does *not* depend on the race conditions between different pseudo-threads).

**Caution:** in addition to any memory overhead listed for Zepto VM-Medium, there is an additional implicit memory overhead associated with PARALLEL instruction: namely, all the states of all the plugin state machines which are run in parallel, need to be kept in RAM simultaneously. Normally, it is not much, but for really constrained environments it might become a problem.

**Note on \| ZEPTOVM_OP_EXPR*_EX \| within PARALLEL pseudo-thread**

EXPRUNOP_EX, EXPRBINOP_EX, and JMPIFEXPR_EX instructions, when applied within PARALLEL pseudo-thread, allow to access original (pre-PARALLEL) expression stack. That is:

* for positive EXPR-OFFSET values, first EXPR-OFFSET values identify expression stack items within the pseudo-thread, but when pseudo-thread values are exhausted, increasing EXPR-OFFSET starts to go into pre-PARALLEL expression stack. 
* negative EXPR-OFFSET values address pre-PARALLEL expression stack (as usual, starting from the bottom of the stack); if pre-PARALLEL expression stack is exhausted, negative EXPR-OFFSET values start to address pseudo-thread's own expression stack
* for all EXPR-OFFSET values, if POP-FLAG is specified and it would affect pre-PARALLEL expression stack, it causes an ZEPTOVM_EXPRSTACKFROZENVIOLATION exception.

TODO: (Medium Level) ZEPTOVM_NETINTERRUPT

TODO: (orthogonal to VM level, starting from Small?) multiplication/division, multiplication/log/exp/sin(?), support for piecewise table maths (with piecewise table supplied as a part of command)

Implementation notes
''''''''''''''''''''

To implement Zepto VM-Medium, in addition to PC, reply-offset-stack, and expression stack as required by Zepto VM-Small, the following changes need to be made:

* PC for each pseudo-threads needs to be maintained; maximum number of pseudo-threads is a ZEPTOVM_MAX_PSEUDOTHREADS parameter of Zepto VM-Medium (which is stored in SmartAnthill DB on SmartAnthill Client and reported via DEVICECAPS instruction).
* expression stack needs to be replaced with an array of expression stacks (to accommodate PARALLEL instruction); in practice, it is normally implemented by extending expression stack (say, doubling it) and keeping track of sub-expression stacks via array of offsets (with size of ZEPTOVM_MAX_PSEUDOTHREADS) within the expression stack. See :ref:`sazeptoos` document for details.
* to support replies being pushed to "reply buffer" in parallel, an additional array of 2-byte offsets of current replies needs to be maintained, with a size of ZEPTOVM_MAX_PSEUDOTHREADS.

Memory overhead
'''''''''''''''

Memory overhead of ZeptoVM-Medium is (in addition to overhead of ZeptoVM-Small) is 1+4*ZEPTOVM_MAX_PSEUDOTHREADS, though an increase of ZEPTOVM_EXPR_STACK_SIZE parameter of ZeptoVM-Small is advised.


Appendix
--------

Statistics for different Zepto-VM levels:

+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Level          |Opcodes Supported|Typical Parameter Values             |Amount of RAM used (with typical parameter values)|
+===============+=================+=====================================+==================================================+
|Zepto VM-One   | TODO            |                                     | 1 to 2                                           |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Zepto VM-Tiny  | TODO            |ZEPTOVM_REPLY_STACK_SIZE=4 to 8      | (1 to 2)+(5 to 9) = 6 to 11                      |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Zepto VM-Small | TODO            |ZEPTOVM_EXPR_STACK_SIZE=4 to 32      | (6 to 11)+(9 to 65) = 15 to 76                   |
|               |                 |ZEPTOVM_EXPR_FLOAT_TYPE=HALF-FLOAT   |                                                  |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Zepto VM-Medium| TODO            |ZEPTOVM_EXPR_STACK_SIZE=32 to 128    | TBD                                              |
|               |                 |ZEPTOVM_MAX_PSEUDOTHREADS=4 to 8     |                                                  |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+

