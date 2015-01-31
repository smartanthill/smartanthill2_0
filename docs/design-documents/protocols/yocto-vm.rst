v0.1.3a

Copyright (c) 2015, OLogN Technologies AG. All rights reserved.

Redistribution and use of this file in source (.rst) and compiled (.html, .pdf, etc.) forms, with or without modification, are permitted provided that the following conditions are met:

  * Redistributions in source form must retain the above copyright notice, this list of conditions and the following disclaimer.

  * Redistributions in compiled form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  * Neither the name of the OLogN Technologies AG nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE

Yocto VM
========

*NB: this document relies on certain terms and concepts introduced in “SmartAnthill Overall Architecture” and “SmartAnthill SACP” documents, please make sure to read them before proceeding.*

Yocto VM is a minimalistic virtual machine used by SmartAnthill Devices. It implements SACP (SmartAnthill Control Protocol) on the side of the SmartAnthill Device (and SACP corresponds to Layer 7 of OSI/ISO network model). By design, Yocto VM is intended to run on devices with extremely limited resources (as little as 512 bytes of RAM).

Sales pitch (not to be taken seriously!)
----------------------------------------

Yocto VM is the only VM which allows you to process fully-fledged Turing-complete byte-code, enables you to program your MCU the way professionals do, with all the bells and whistles such as flow control (including both conditions and loops), postfix expressions, subroutine calls, C routine calls, MCU sleep mode (where not prohibited by law of physics), and even a reasonable facsimile of “green threads” - all at a miserable price of 1 to 50 bytes of RAM (some restrictions apply, batteries not included). Yes, today you can get many of these features at the price of 1 (one) byte of RAM (offer is valid while supplies last, stores open late). 

We're so confident in our product that we offer a unique memory-back guarantee for 30 days or 30 seconds, whichever comes first. Yes, if you are not satisfied with Yocto VM and remove it from your MCU, you'll immediately get all your hard earned bytes back, no questions asked, no strings attached. 
TODO: proof of being Turing-complete via being able to implement brainfuck

Yocto VM Philosophy
-------------------

“Yocto” is a prefix in the metric system (SI system) which denotes a factor of 10^-24. This is 10^15 times less than “nano”, 10^12 times less than “pico”, and a billion times less than “femto”. As of now, 'yocto' is the smallest prefix in SI system. 

Yocto VM is the smallest VM we were able to think about, with an emphasis of using as less RAM as possible. While in theory it might be possible to implement something smaller, in practice it is difficult to go below 1 byte of RAM (which is the minimum overhead by Yocto VM-One).

To handle plugins and replies, Yocto VM uses a “reply buffer”. Whenever plugin is called, it is asked to place its reply at the end of “reply buffer”. Therefore, if there is more than one instruction, plugin replies are effectively collected in a “reply buffer” (in the order of instructions). As “reply buffer” would be needed regardless of Yocto VM (even simple call to a plugin would need to implement some kind of “reply buffer”), it is not considered a part of Yocto VM and it's size is not counted as “memory overhead” of Yocto VM. 

Note on memory overhead
^^^^^^^^^^^^^^^^^^^^^^^

While Yocto VM itself indeed uses ridiculously low amount of RAM, a developer needs to understand that using some capabilities of Yocto VM will implicitly require more RAM. For example, stacking several replies in one packet will implicitly require more RAM for “reply buffer”. And using “green pseudo-threads” feature will require to store certain portions of the intermediate state of the plugins running simultaneously, at the same time (while without “green pseudo-threads” this RAM can be reused, so the intermediate state of only one plugin needs to be stored at a time).

Yocto VM Restrictions
---------------------

As Yocto VM implements an “Execution Layer” of SACP, it needs to implement all  “Execution Layer Restrictions” set in “SmartAnthill SACP” document. While present document doesn't duplicate these restrictions, it aims to specify them in appropriate places (for example, when specific instructions are described). 

“Program Errors” as specified in Execution Layer Restrictions are implemented as YOCTOVM_PROGRAMERROR_* Yocto VM exceptions as described below.

Bodyparts and Plugins
---------------------

According to a more general SmartAnthill architecture, each SmartAnthill Device (a.k.a. 'Ant') has one or more sensors and/or actuators, with each sensor or actuator known as an 'ant body part'. Each 'body part' is assigned it's own id, which is stored in 'SmartAnthill Database' within SmartAnthill Central Controller.
For each body part type, there is a 'plugin' (so if there are body parts of the same type in the device, number of plugins can be smaller than number of body parts). Plugins are pieces of code which are written in C language and programmed into MCU of SmartAnthill device. 

Structure of Plugin Data and Error Codes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Data to be passed to and from plugins is generally opaque, with one exception: first byte of plugin reply is interpreted as an unsigned 'error code'. Exact meaning of 'error code' is plugin-specific, but in general it should be thought of as something similar to 'program exit code' in traditional OSes.

Error code == 255 is reserved for Yocto VM exceptions (see below) and SHOULD NOT be returned by conforming plugins.

Packet Chains
-------------

In SACP (and in Yocto VM as an implementation of SACP), all interactions between SmartAnthill Central Controller and SmartAnthill Device are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on. Whenever Yocto VM issues a packet to an underlying protocol, it needs to specify whether a packet is a first, intermediate, or last within a “packet chain” (using 'is-first' and 'is-last' flags; note that due to “rules of engagement” described below, 'is-first' and 'is-last' flags are inherently incompatible, which MAY be relied on by implementation). This information allows underlying protocol to arrange for proper retransmission if some packets are lost during communication. See "SmartAnthill Protocol Stack" document for more details on "packet chains". 

Yocto VM Instructions
---------------------

All Yocto VM instructions have the same basic format (through this document '\|' denotes field boundaries):
\| OP-CODE \| OP-PARAMS \|
where OP-CODE is a 1-byte operation code, and length and content of OP-PARAMS are implicitly defined by OP code. 
If one of OP-PARAM fields is separated into bitfields, it is denoted as \| SOME_PARAM,SOME_OTHER_PARAM\| and exact length of bitfields is specified in instruction description.

List of Yocto VM opcodes:

* YOCTOVM_OP_EXEC
* YOCTOVM_OP_PUSHREPLY
* YOCTOVM_OP_SLEEP
* YOCTOVM_OP_MCUSLEEP
* YOCTOVM_OP_POPREPLIES
* YOCTOVM_OP_EXIT
* YOCTOVM_OP_JMP */\* starting from this opcode, instructions are not supported by Yocto VM-One \*/*
* YOCTOVM_OP_JMPIFERRORCODE_LT
* YOCTOVM_OP_JMPIFERRORCODE_GT
* YOCTOVM_OP_JMPIFERRORCODE_EQ
* YOCTOVM_OP_PUSHEXPR_CONSTANT */\* starting from this opcode, instructions are not supported by Yocto VM-Tiny and below \*/*
* YOCTOVM_OP_PUSHEXPR_ERRORCODE
* YOCTOVM_OP_PUSHEXPR_1BYTE_FROMREPLY
* YOCTOVM_OP_PUSHEXPR_2BYTES_FROMREPLY
* YOCTOVM_OP_PUSHEXPR_EXPR
* YOCTOVM_OP_POPEXPR
* YOCTOVM_OP_EXPRUNOP
* YOCTOVM_OP_EXPRBINOP
* YOCTOVM_OP_JMPIFEXPR_LT
* YOCTOVM_OP_JMPIFEXPR_GT
* YOCTOVM_OP_JMPIFEXPR_EQ
* YOCTOVM_OP_JMPIFEXPR_NE
* YOCTOVM_OP_JMPIFEXPR_NOPOP_LT
* YOCTOVM_OP_JMPIFEXPR_NOPOP_GT
* YOCTOVM_OP_JMPIFEXPR_NOPOP_EQ
* YOCTOVM_OP_JMPIFEXPR_NOPOP_NE
* YOCTOVM_OP_PARALLEL */\* starting from this opcode, instructions are not supported by Yocto VM-Small and below \*/*

Yocto VM Exceptions
-------------------

If Yocto VM encounters a problem, it reports it as an “VM exception”. Whenever exception characterized by EXCEPTION-CODE occurs, it is processed as follows:

* all contents of “reply buffer” is discarded
* “reply buffer” is filled with the following information: \|255\|EXCEPTION-CODE\|INSTRUCTION-POSITION\| , where all fields are 1-byte.
* This reply is sent back to the command originator.

The structure of the reply means that it will be interpreted as a reply with error code == 255, and by convention error code 255 is reserved for VM exceptions.

Currently, Yocto VM may issue the following exceptions:

* YOCTO_VM_INVALID_INSTRUCTION */\* Note that this exception may also be issued when an instruction is encountered which is legal in general, but is not supported by current level of Yocto VM. \*/*
* YOCTOVM_PLUGINERROR
* YOCTOVM_INVALIDPARAMETER
* YOCTOVM_INVALIDREPLYOFFSET
* YOCTOVM_EXPRSTACKUNDERFLOW
* YOCTOVM_EXPRSTACKINVALIDINDEX
* YOCTOVM_EXPRSTACKOVERFLOW
* YOCTOVM_PROGRAMERROR_INVALIDREPLYFLAG
* YOCTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE

Yocto VM End of Execution
-------------------------

Yocto VM program exits when the sequence of instructions has ended. At this point, an equivalent of \|EXIT\|ISLAST\| is implicitly executed (see description of 'EXIT' instruction below); this causes “reply buffer” to be sent back to the SmartAnt Central Controller, with 'is-last' flag set. Alternatively, an “EXIT” command (see below) may end program execution; in this case, parameter to “EXIT” command specifies which flag is to be used.

Yocto VM Overriding Command
---------------------------

If there is a new command incoming from SmartAnthill Central Controller, while Yocto VM is executing a current program, Yocto VM will (at the very first opportunity) automatically abort execution of the current program, and starts executing the new one. This behaviour is consistent with the concept of “SmartAnthill Central Controller always knows better” which is used throughout the SmartAnthill protocol stack. Such command may be used, for example, by SmartAnthill Central Controller to abort execution of a long-running request and ask SmartAnthill Device to do something else.

Yocto VM Levels
---------------

To accommodate SmartAnthill devices with different capabilities and different amount of RAM, Yocto VM implementations are divided into several levels. Minimal level, which is mandatory for all implementations of Yocto VM, is Level One. Each subsequent Yocto VM level adds support for some new instructions while still supporting all the capabilities of underlying levels.

TODO: timeouts

Level One
^^^^^^^^^

YoctoVM-One is the absolute minimum implementation of Yocto-VM, which allows to execute only a linear sequence of commands, at the cost of additional RAM needed being 1 byte. YoctoVM-One supports the following instructions:

**\| YOCTOVM_OP_EXEC \| BODYPART-ID \| DATA-SIZE \| DATA \|**

where YOCTOVM_OP_EXEC is 1-byte opcode, BODYPART-ID is 1-byte id of the bodypart to be used, DATA-SIZE is a 1-byte length of DATA field, and DATA in an opaque data to be passed to the plugin associated with body part identified by BODYPART-ID; DATA field has size DATA-SIZE.
EXEC instruction invokes a plug-in which corresponds to BODYPART-ID, and passes DATA of DATA-SIZE  size to this plug-in. Plug-in always adds a reply to the reply-buffer; reply size may vary, but MUST be at least 1 byte in length; otherwise it is a YOCTOVM_PLUGINERROR exception.

**\| YOCTOVM_OP_PUSHREPLY \| DATA-SIZE \| DATA \|**

where YOCTOVM_OP_PUSHREPLY is a 1-byte opcode, DATA-SIZE is a 1-byte length of DATA field, and DATA is opaque data to be pushed to reply buffer.
PUSHREPLY instruction pushes an additional reply with DATA in it to reply buffer.

**\| YOCTOVM_OP_SLEEP \| MSEC-DELAY \|**

where YOCTOVM_OP_SLEEP is a 1-byte opcode, and MSEC-DELAY is a 2-byte unsigned integer.
Pauses execution for approximately MSEC-DELAY milliseconds.

**\| YOCTOVM_OP_MCUSLEEP \| SEC-DELAY \|**

where YOCTOVM_OP_MCUSLEEP is a 1-byte opcode, and SEC-DELAY is a 2-byte unsigned integer.
MCUSLEEP instruction puts MCU into sleep-with-timer mode for approximately SEC-DELAY seconds. If sleep-with-timer mode is not available with current MCU, then such an instruction still may be sent to such a device, as a means of long delay, and SmartAnthill device MUST process it just by waiting for specified time. 
As MCUSLEEP may disable device receiver, Yocto VM enforces relevant “Execution Layer Restrictions” when MCUSLEEP is invoked; to ensure consistent behavior between MCUs, these restriction MUST be enforced regardless of MCUSLEEP really disabling device receiver. Therefore (NB: these checks SHOULD be implemented for YoctoVM-One; they MUST be implemented for all Yocto-VM levels other than YoctoVM-One):

* If original command has not had an ISLAST flag, and MCUSLEEP is invoked, it is YOCTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE exception.
* Yocto VM keeps track if MCUSLEEP was invoked; this 'mcusleep-invoked' flag is used by some other instructions.
* NB: double MCUSLEEP within the same program is ok, so if 'mcusleep-invoked' flag is already set and MCUSLEEP is invoked, this is not a problem

It should be noted that implementing MCUSLEEP instruction will implicitly require storing current PC and current “reply buffer” either in EEPROM, or to request MPU to preserve RAM while waiting. This will be done automagically by Yocto VM, but it is not without it's cost. It might be useful to know that in some cases this cost is lower when amount of data to be preserved is small (for example, it happens when “reply buffer” is empty).


**\| YOCTOVM_OP_POPREPLIES \| N-REPLIES \|**

where YOCTOVM_OP_POPREPLIES is a 1-byte opcode (NB: it is the same as YOCTOVM_OP_POPREPLIES in Level Tiny), and N-REPLIES is 1 byte, which MUST be 255 for Yocto VM-One (other values are allowed for Yocto VM-Tiny and above, as described below). If N-REPLIES is not 255 for Yocto VM-One POPREPLIES instruction, Yocto VM will issue a YOCTOVM_INVALIDPARAMETER exception. \|POPREPLIES\|255\| effectively means “remove all replies currently in reply buffer”.

NB: Yocto VM-One implements POPREPLIES instruction only partially (for 1 value of N-REPLIES); Yocto VM-Tiny supports other values as described below, and behavior for this 1 value of N-REPLIES which is supported by both Yocto VM-One and Yocto VM-Tiny is consistent for any Yocto VM implementation.

**\| YOCTOVM_OP_EXIT \| <REPLY-FLAGS> \|**

where YOCTOVM_OP_EXIT is a 1-byte opcode (NB: it is the same as YOCTOVM_OP_EXIT in Level Tiny), and REPLY-FLAGS is a 1-byte flag taking one of the following values: {NONE,ISFIRST,ISLAST}
EXIT instruction posts all the replies in the “reply buffer” and terminates the program. Device receiver is kept turned on after the program exits (so the device is able to accept new commands).

To enforce “Execution Layer Requirements”, the following SHOULD be enforced for Yocto VM-One and MUST be enforced for other Yocto VM layers:

* if 'mcusleep-invoked' flag is not set, and original command has had ISLAST flag, then “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS != ISFIRST (this is an usual command-reply pattern)
* if 'mcusleep-invoked' flag is not set, and original command has not had ISLAST flag, then “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS == ISFIRST (this is a 'long command-reply' pattern)
* if 'mcusleep-invoked' flag is set, then original command will have ISLAST flag (because of other restrictions; this means violating 'ISLAST' requirement while processing EXIT instruction is not an exception, but an internal assertion which MUST NOT happen); “reply buffer” MUST be non-empty, and EXIT instruction MUST have REPLY-FLAGS == ISFIRST (this is a 'mcusleep-then-wake' pattern)

If any of the restrictions above is not compied with, Yocto VM generates a YOCTOVM_PROGRAMERROR_INVALIDREPLYSEQUENCE exception.

Implementation notes
''''''''''''''''''''

If strict checks of “Execution Layer Restrictions” are disabled (which is allowed only for Yocto VM-One and not for any other level), then only PC (Program Counter) needs to be maintained for operating Level One.

To keep track of “Execution Layer Restrictions”, a one-byte flag bitmask is used with the following flags:

* mcusleep-invoked
* *currently there are no other flags*

Memory overhead
'''''''''''''''

Memory overhead of YoctoVM-One is 1 byte; if “Execution Layer Restrictions” are strictly enforced (which is a MUST for all levels except for Yocto VM-One), this requires an additional 1 byte.

Level Tiny
^^^^^^^^^^

Yocto VM-Tiny allows for more complicated programs, including basic conditions, at the cost of additional memory needed being on the order of 5-10 bytes. Yocto VM-Tiny, in addition to instructions supported by Yocto VM-One, additionally supports the following instructions:

**\| YOCTOVM_OP_JMP \| DELTA \|**

where YOCTOVM_OP_JMP is a 1-byte opcode, and DELTA is a 1-byte signed integer which denotes how PC (program counter) should be changed (DELTA is considered in relation to the end of JMP instruction, so JMP 0 is effectively a no-op).

**\| YOCTOVM_OP_JMPIFERRORCODE_<SUBCODE> \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ}; YOCTOVM_OP_JMPIFERRORCODE_LT, YOCTOVM_OP_JMPIFERRORCODE_GT, and  YOCTOVM_OP_JMPIFERRORCODE_EQ are 1-byte opcodes, THRESHOLD is a 1-byte unsigned integer, and interpretation of DELTA is similar to that of in JMP instruction description.

YOCTOVM_OP_JMPIFERRORCODE_* instruction takes the reply of the last plugin which was called, and compares first byte of the reply (which by convention represents 'plugin error code', see above) to the THRESHOLD. If first byte of the reply is < (for <SUBCODE>=LT) THRESHOLD, PC is incremented by a value of DELTA (as with JMP, DELTA is added to a PC positioned right after current instruction).

+---------+---------------------------------------+
|<SUBCODE>|Jump if                                |
+=========+=======================================+
|LT       | First byte of last reply < THRESHOLD  |
+---------+---------------------------------------+
|GT       | First byte of last reply > THRESHOLD  |
+---------+---------------------------------------+
|EQ       | First byte of last reply == THRESHOLD |
+---------+---------------------------------------+

**\| YOCTOMV_OP_POPREPLIES \| N-REPLIES \|**

where POPREPLIES is a 1-byte opcode and N-REPLIES is a 1-byte number of replies to be popped. 

POPREPLIES instruction removes last N-REPLIES of plugins from the reply buffer. If N-REPLIES is less than number of replies currently in buffer, it means that all replies are removed, therefore \|POPREPLIES\|255\| always means “Remove all replies currently in reply buffer”. Usually, either \|POPREPLIES\|1\| or \|POPREPLIES\|255\| is used, but other values are also possible.

Implementation notes
''''''''''''''''''''

To implement Yocto VM-Tiny, in addition to PC required by Yocto VM-One, a stack of offsets which signify positions of recent replies in “reply buffer”, need to be maintained. Such stack should consist of an array of bytes for offsets, and additional byte to store number of entries on the stack. Size of this stack is a YOCTOVM_REPLY_STACK_SIZE parameter of Yocto VM-Tiny (which is stored in SmartAnthill DB on SmartAnthill Central Controller).

Memory overhead
'''''''''''''''

Memory overhead of YoctoVM-Tiny is (in addition to overhead of YoctoVM-One) is 1+YOCTOVM_REPLY_STACK_SIZE. 

Level Small
^^^^^^^^^^^

Yocto VM-Small allows for even more complicated programs, including expressions and loops, at che cost of additional memory needed (in addition to Yocto VM-Tiny) being on the order of 9-17 bytes.
Yocto VM-Small, in addition to instructions supported by Yocto VM-Tiny, additionally supports the following instructions:

**\| YOCTOVM_OP_PUSHEXPR_CONSTANT \| CONST \|**

where where YOCTOVM_OP_PUSHEXPR_CONSTANT is 1-byte opcode, and CONST is a 2-byte constant to be pushed to expression stack.

PUSHEXPR_CONSTANT instruction pushes CONST to an expression stack (if expression stack is exceeded, it will cause YOCTOVM_EXPRSTACKOVERFLOW VM exception).

**\| YOCTOVM_OP_PUSHEXPR_ERRORCODE \| REPLY-OFFSET \|**

where YOCTOVM_OP_PUSHEXPR_ERRORCODE is 1-byte opcode, and REPLY-OFFSET is a 1-byte offset of reply in “reply buffer”, so that REPLY-OFFSET == 0 corresponds to most recent reply,  REPLY-OFFSET == 1 corresponds to a previous one and so on. If REPLY-OFFSET is more than current value of replies in “reply buffer”, this will cause a YOCTOVM_INVALIDREPLYOFFSET VM exception.

PUSHEXPR_ERRORCODE pushes an error code of appropriate reply (as specified by REPLY-OFFSET, see details above) to the expression stack (if expression stack is exceeded, it will cause YOCTOVM_EXPRSTACKOVERFLOW VM exception).

**\| YOCTOVM_OP_PUSHEXPR <LEN> FROMREPLY \| REPLY-OFFSET \| OFFSET-WITHIN-REPLY \|**

where <LEN> is one of {1BYTE,2BYTES}; YOCTOVM_OP_PUSHEXPR_1BYTE_FROMREPLY and  YOCTOVM_OP_PUSHEXPR_2BYTES_FROMREPLY are 1-byte opcodes, REPLY-OFFSET is a 1-byte offset similar to that of PUSHEXPR_ERRORCODE, and OFFSET-WITHIN-REPLY is a 1-byte offset within specified reply.  If REPLY-OFFSET is more than current value of replies in “reply buffer”, this will cause a YOCTOVM_INVALIDREPLYOFFSET VM exception.

PUSHEXPR <LEN> FROMREPLY takes one or two bytes (as specified by <LEN>) from reply specified by REPLY-OFFSET, at offset within reply as specified by OFFSET-WITHIN-REPLY, and pushes it to the expression stack (if expression stack is exceeded, it will cause YOCTOVM_EXPRSTACKOVERFLOW VM exception). 
The idea of the PUSHEXPR <LEN> FROMREPLY instruction is that, assuming that one knows the format of reply, she can extract multiple parameters from the replies. Note that due to convention that first byte of reply is the errorcode, \|PUSHEXPR_1BYTE_FROMREPLY\|REPLY-OFFSET\|0\| is the same as \|PUSHEXPR_ERRORCODE\|REPLY-OFFSET\|.

**\ YOCTOVM_OP_PUSHEXPR_EXPR \| EXPR-OFFSET \|** 

where YOCTOVM_OP_PUSHEXPR_EXPR is a 1-byte opcode, and EXPR-OFFSET is a 1-byte offset of the expression which needs to be duplicated on the top of the expression stack.

PUSHEXPR_EXPR instruction peeks a value from the expression stack without removing it from the stack; the value is specified by EXPR-OFFSET, so that EXPR-OFFSET == 0 means "topmost value on the stack", EXPR-OFFSET == 1 means "second topmost value on the stack" and so on. If EXPR-OFFSET is greater than current expression stack size, this will cause YOCTOVM_EXPRSTACKINVALIDINDEX exception.

PUSHEXPR_EXPR instruction is mostly useful within PARALLEL environments (see note on it's specifics in description of YoctoVM-Medium), but is supported in YoctoVM-Small too.

**\| YOCTOVM_OP_POPEXPR \|**

where YOCTOVM_OP_POPEXPR is a 1-byte opcode

POPEXPR instruction removes the topmost value from the expression stack.

**\| YOCTOVM_OP_EXPRUNOP \| UNOP \|**

where YOCTOVM_OP_EXPRUNOP is a 1-byte opcode, and UNOP is 1-byte taking values from 0 to 4:

+----+-------------------------------+
|UNOP|Corresponding unary C operation|
+====+===============================+
|0   + \-                            |
+----+-------------------------------+
|1   + ~                             |
+----+-------------------------------+
|2   + !                             |
+----+-------------------------------+
|3   + ++                            |
+----+-------------------------------+
|4   + --                            |
+----+-------------------------------+

EXPRUNOP instruction pops topmost value from the expression stack, modifies it according to the table above, and pushes modified value back to expression stack. All operations are performed as specified in the table above, using signed 16-bit arithmetic. If expression stack is empty, it will cause a YOCTOVM_EXPRSTACKUNDERFLOW VM exception. TODO? : overflows for '-','++','--'? 

**\| YOCTOVM_OP_EXPRBINOP \| BINOP \|**

where YOCTOVM_OP_EXPRBINOP is a 1-byte opcode, and BINOP is 1-byte taking values from 0 to 7:

+-----+--------------------------------+
|BINOP|Corresponding binary C operation|
+=====+================================+
|0    + \+                             |
+-----+--------------------------------+
|1    + \-                             |
+-----+--------------------------------+
|2    + <<                             |
+-----+--------------------------------+
|3    + <<                             |
+-----+--------------------------------+
|4    + &                              |
+-----+--------------------------------+
|5    + \|                             |
+-----+--------------------------------+
|6    + &&                             |
+-----+--------------------------------+
|7    + ||                             |
+-----+--------------------------------+

EXPRBINOP instruction pops two topmost values from the expression stack, calculates result out of them according to the table above (as 'second topmost' op 'topmost'), and pushes calculated value back to the expression stack. All operations are performed as specified in the table above, using signed 16-bit arithmetic (except for shifts, which use unsigned 16-bit arithmetic). If expression stack has less than two items, it will cause a YOCTOVM_EXPRSTACKUNDERFLOW VM exception. TODO? : overflows for '+','-','<<'? 

**\| YOCTOVM_OP_JMPIFEXPR <SUBCODE> \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ,NE}; YOCTOVM_OP_JMPIFEXPR_LT, YOCTOVM_OP_JMPIFEXPR_GT, YOCTOVM_OP_JMPIFEXPR_EQ, and  YOCTOVM_OP_JMPIFEXPR_NE are 1-byte opcodes, THRESHOLD is a 2-byte signed integer, and interpretation of DELTA is similar to that of in JMP description. 

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

JMPIFEXPR <SUBCODE> instruction pops the topmost value from the expression stack, compares it with THRESHOLD according to <SUBCODE>, and updates Program Counter by DELTA if condition specified by comparison is met (as with JMP, DELTA is added to a PC positioned right after current instruction). If expression stack is empty, it will cause a YOCTOVM_EXPRSTACKUNDERFLOW VM exception. 

**\| YOCTOVM_OP_JMPIFEXPR_NOPOP <SUBCODE> \| THRESHOLD \| DELTA \|**

where <SUBCODE> is one of {LT,GT,EQ,NE}; YOCTOVM_OP_JMPIFEXPR_NOPOP_LT, YOCTOVM_OP_JMPIFEXPR_NOPOP_GT, YOCTOVM_OP_JMPIFEXPR_NOPOP_EQ, and  YOCTOVM_OP_JMPIFEXPR_NOPOP_NE are 1-byte opcodes, THRESHOLD is a 2-byte signed integer, and interpretation of DELTA is similar to that of in JMP description. 

JMPIFEXPR_NOPOP <SUBCODE> instruction peeks the topmost value on the expression stack without popping it, compares it with THRESHOLD according to <SUBCODE>, and updates Program Counter by DELTA if condition specified by comparison is met (as with JMP, DELTA is added to a PC positioned right after current instruction). If expression stack is empty, it will cause a YOCTOVM_EXPRSTACKUNDERFLOW VM exception. For details on <SUBCODE>, see description of JMPIFEXPR <SUBCODE> instruction.

JMPIFEXPR_NOPOP instruction is useful for organizing loops based on a value stored on the expression stack: for example, sequence such as \|EXPRUNOP\|++\|JMPIFEXPR NOPOP LT\|5\|NEGATIVE-DELTA\| can be used at the end of the do{...;i++;}while(i<5); loop (use within while and for loops is similar).

Implementation notes
''''''''''''''''''''

To implement Yocto VM-Small, in addition to PC and reply-offset-stack required by Yocto VM-Tiny, an expression stack of 16-bit values, need to be maintained. Such stack should consist of an array of 16-bit values, and additional byte to store number of entries on the stack. Size of this stack is a YOCTOVM_EXPR_STACK_SIZE parameter of Yocto VM-Small (which is stored in SmartAnthill DB on SmartAnthill Central Controller).

Memory overhead
'''''''''''''''

Memory overhead of YoctoVM-Small is (in addition to overhead of YoctoVM-Tiny) is 1+2*YOCTOVM_EXPR_STACK_SIZE. 

Yocto VM-Medium
^^^^^^^^^^^^^^^

Yocto VM-Medium adds support for registers, call stack, and parallel execution.

**\| YOCTOVM_OP_PARALLEL \| N-PSEUDO-THREADS \| PSEUDO-THREAD-1-INSTRUCTIONS-SIZE \| PSEUDO-THREAD-1-INSTRUCTIONS \| ... \| PSEUDO-THREAD-N-INSTRUCTIONS-SIZE \| PSEUDO-THREAD-N-INSTRUCTIONS \|**

where YOCTOVM_OP_PARALLEL is 1-byte opcode, N-PSEUDO-THREADS is a number of "pseudo-threads" requested, 'PSEUDO-THREAD-X-INSTRUCTIONS-SIZE' is 1-byte size of PSEUDO-THREAD-X-INSTRUCTIONS, and PSEUDO-THREAD-X-INSTRUCTIONS is a sequence of Yocto VM commands which belong to the pseudo-thread #X. Within PSEUDO-THREAD-X-INSTRUCTIONS, all commands of Yocto VM are allowed, with an exception of PARALLEL, EXIT and any jump instruction which leads outside of the current pseudo-thread.

PARALLEL instruction starts processing of several pseudo-threads. PARALLEL instruction is considered completed when all the pseudo-threads reach the end of their respective instructions. Normally, it is implemented via state machines (see "SmartAnthill Reference Implementation - MCU Software Architecture" document for details), so it is functionally equivalent to "green threads" (and not to "native threads").

When PARALLEL instruction execution is started, original "reply buffer" is "frozen" and cannot be accessed by any of the pseudo-threads; each pseudo-thread has it's own "reply buffer" which is empty at the beginning of the pseudo-thread execution. After PARALLEL instruction is completed (i.e. all pseudo-threads have been terminated), the original "reply buffer" which existed before PARALLEL instruction has started, is restored, and all the pseudo-thread "reply buffers" which existed right before after respective pseudo-threads are terminated, are added to the end of the original "reply buffer"; this allows to have instructions such as EXEC and PUSHREPLY within the pseudo-threads; this adding of pseudo-thread "reply buffers" to the end of original "reply buffer" always happens in order of pseudo-thread descriptions within the PARALLEL instruction (and is therefore does *not* depend on the race conditions between different pseudo-threads).

When PARALLEL instruction execution is started, original expression stack is "frozen" and cannot be manipulated by any of the pseudo-threads (though it may be read using PUSHEXPR_EXPR instruction as described below); each pseudo-thread has it's own expression stack which is empty at the beginning of the pseudo-thread execution. After PARALLEL instruction is completed (i.e. all pseudo-threads have been terminated), the original expression stack which existed before PARALLEL instruction has started, is restored, and all the pseudo-thread expression stacks remaining after respective pseudo-threads are terminated, are added to the top of this original stack; this allows to easily pass information from pseudo-threads to the main program; this adding of pseudo-thread expression stacks on top of original expression stack always happens in order of pseudo-thread descriptions within the PARALLEL instruction (and is therefore does *not* depend on the race conditions between different pseudo-threads).

**Caution:** in addition to any memory overhead listed for Yocto VM-Medium, there is an additional implicit memory overhead associated with PARALLEL instruction: namely, all the states of all the plugin state machines which are run in parallel, need to be kept in RAM simultaneously. Normally, it is not much, but for really constrained environments it might become a problem.

**Note on \ YOCTOVM_OP_PUSHEXPR_EXPR \| EXPR-OFFSET \| within PARALLEL pseudo-thread** 

PUSHEXPR_EXPR instruction, when it is applied within PARALLEL pseudo-thread, allows to access original (pre-PARALLEL) expression stack. That is, first EXPR-OFFSET values identify expression stack items within the pseudo-thread, but when pseudo-thread values are exhausted, increasing EXPR-OFFSET starts to go into pre-PARALLEL expression stack. For example, if \|PUSHEXPR\|0\| is the first instruction of the pseudo-thread, it peeks a topmost value from the pre-PARALLEL expression stack and pushes it to the pseudo-thread's expression stack. This allows to easily pass information from the main program to pseudo-threads.

TODO: CALL (accounting for pseudo-threads), MOV (pseudo-threads-agnostic)

Implementation notes
''''''''''''''''''''

To implement Yocto VM-Medium, in addition to PC, reply-offset-stack, and expression stack as required by Yocto VM-Small, the following changes need to be made: 

* PC for each pseudo-threads needs to be maintained; maximum number of pseudo-threads is a YOCTOVM_MAX_PSEUDOTHREADS parameter of Yocto VM-Medium (which is stored in SmartAnthill DB on SmartAnthill Central Controller).
* expression stack needs to be replaced with an array of expression stacks (to accommodate PARALLEL instruction); in practice, it is normally implemented by extending expression stack (say, doubling it) and keeping track of sub-expression stacks via array of offsets (with size of YOCTOVM_MAX_PSEUDOTHREADS) within the expression stack. See "SmartAnthill Reference Implementation - MCU Software Architecture" document for details.
* to support replies being pushed to "reply buffer" in parallel, an additional array of 2-byte offsets of current replies needs to be maintained, with a size of YOCTOVM_MAX_PSEUDOTHREADS.

Memory overhead
'''''''''''''''

Memory overhead of YoctoVM-Medium is (in addition to overhead of YoctoVM-Small) is 1+4*YOCTOVM_MAX_PSEUDOTHREADS, though if PARALLEL instruction is intended to be used, an increase of YOCTOVM_EXPR_STACK_SIZE parameter of YoctoVM-Small is advised.

TODO: YOCTOVM_INTERRUPT (? where?)

Appendix
--------

Statistics for different Yocto-VM levels:

+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Level          |Opcodes Supported|Typical Parameter Values             |Amount of RAM used (with typical parameter values)|
+===============+=================+=====================================+==================================================+
|Yocto VM-One   | 6               |                                     | 1 to 2                                           |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Yocto VM-Tiny  | 10              |YOCTOVM_REPLY_STACK_SIZE=4 to 8      | (1 to 2)+(5 to 9) = 6 to 11                      |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Yocto VM-Small | 26              |YOCTOVM_EXPR_STACK_SIZE=4 to 8       | (6 to 11)+(9 to 17) = 15 to 28                   |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+
|Yocto VM-Medium| 27+TBD          |YOCTOVM_EXPR_STACK_SIZE=8 to 12      | TBD                                              |
|               |                 |YOCTOVM_MAX_PSEUDOTHREADS=4 to 8     |                                                  |
+---------------+-----------------+-------------------------------------+--------------------------------------------------+

