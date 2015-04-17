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

.. _sascrambling:

SmartAnthill SCRAMBLING procedure
=================================

:Version:   v0.5.2

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SmartAnthill SCRAMBLING procedure aims to provide some extra protection when data is transmitted in open (in particular, over wireless or over the Internet). **SCRAMBLING procedure does not provide security guarantees** in a strict sense, but might hide certain details (such as source/destination addresses) and does help against certain classes of DoS attacks. Because of the lack of security guarantees, SCRAMBLING procedure SHOULD NOT be used as a sole encryption protocol (using it over SASP is fine).

SCRAMBLING procedure requires both sides to share one AES-128 key. **SCRAMBLING key MUST be separate and independent from any other key in the system, in particular, from SASP keys.**

SCRAMBLING procedure is intended to be used as the outermost packet wrapper which is possible for an underlying protocol. Within SmartAnthill Protocol Stack, SCRAMBLING procedure is OPTIONALLY used by SAoIP protocol as described in :ref:`saoip` document. In addition, SADLP-\* protocols, especially those working over wireless L1 protocols, SHOULD use SCRAMBLING procedure to hide as much information as possible. 

.. contents::

Environment
-----------

SCRAMBLING procedure is a procedure of taking an input packet of arbitrary size, and producing a "scrambled" packet. It is used both by SAoIP and some of SADLP-\*.

SCRAMBLING procedure requires both sides to share one secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

For SCRAMBLING procedure to be efficient (in secure sense), caller SHOULD guarantee that there is a 15-byte block within input packet, where such block is at least statistically unique, and the same block is statistically indistinguishable from white noise. Offset to such a block within the packet is an input *unique-block-offset* parameter for SCRAMBLING procedure. In practice, 15 bytes of SASP tag are used for this purpose.

SCRAMBLING procedure
--------------------

Input
^^^^^

Input of SCRAMBLING procedure is a pre-SCRAMBLING packet, and *unique-block-offset* offset. pre-SCRAMBLING packet can be considered as follows:

**\| unencrypted-pre-SCRAMBLING-Data \| encrypted-pre-unique-pre-SCRAMBLING-Data \| encrypted-unique-block \| encrypted-post-unique-pre-SCRAMBLING-Data \|**

where encrypted-unique-block is always 15 bytes in size, and it's offset from the beginning is specified by *unique-block-offset* input parameter, and any of encrypted-pre-unique-pre-SCRAMBLING-Data and encrypted-post-unique-pre-SCRAMBLING-Data can have 0 size.

*unique-block-offset+15* MUST be within pre-SCRAMBLING-Data.

Procedure
^^^^^^^^^

SCRAMBLING procedure works as follows:

1. Form SCRAMBLING-Header according to formatting schema (Default schema is described below, but SADLP-* implementations are allowed to define their own schemas if necessary).

SCRAMBLING-Header, regardless of formatting schema, MUST specify Scrambled-Size and Forced-Padding-Size parameters. Scrambled-Size is a number of 16-byte blocks which were scrambled; *16\*Scrambled-Size* MUST be >= size of SCRAMBLING-Header. For security purposes, sender MAY scramble more bytes (and respectively specify Scrambled-Size) than strictly necessary. However, sender MUST NOT specify Scrambled-Size so that *16\*Scrambled-Size* is more than `sizeof(SCRAMBLING-Header)+sizeof(encrypted-pre-unique-pre-SCRAMBLING-Data)+sizeof(encrypted-post-unique-pre-SCRAMBLING-Data)+15`; otherwise, receiver MUST treat it as a malformed packet. 

2. Form pre-SCRAMBLED packet which has the following format:

**\| encrypted-unique-block \| SCRAMBLED-Header \| encrypted-pre-unique-pre-SCRAMBLING-Data \| encrypted-post-unique-pre-SCRAMBLING-Data \| Optional-Forced-Padding \|**

where Optional-Forced-Padding is optional forced padding, which has size of Forced-Padding-Size parameter from SCRAMBLING-Header. Forced-Padding, if present, MUST be generated using SmartAnthill Non-Key Random Stream (which is described in :ref:`sarng`).

3. Encrypt a portion of pre-SCRAMBLED packet, starting from SCRAMBLED-HEADER, and with length of Scrambled-Size*16 (as specified in SCRAMBLING-Header), using AES-128 in CTR mode, using SCRAMBLING key, and using `( encrypted-unique-block << 8 )` as initial counter for CTR. CTR mode, combined with statistical-uniqueness requirement for unique-block, ensures that SCRAMBLED data is indistinguishable from white noise for a potential attacker. NB: size of `( encrypted-unique-block << 8 )` is 128 bit, or one AES-128 block. NB2: this construct restrict the size of unencrypted-pre-SCRAMBLING-Data to 16*256=4096 bytes; it is orders of magnitude larger than any practical headers may reasonably require. 

If *16\*Scrambled-Size* goes beyond encrypted-post-unique-pre-SCRAMBLING-DATA, remaining SCRAMBLING bytes are ignored; due to requirement on Scrambled-Size stated above, number of such ignored bytes cannot exceed 15.


Default SCRAMBLING-Header Schema
''''''''''''''''''''''''''''''''

Default SCRAMBLING-Header Schema assumes that the size of encrypted-post-unique-pre-SCRAMBLING-Data is always zero (and that therefore *unique-block-offset* parameter is always equal to `pre_SCRAMBLING_packet_size-15`). This occurs when (a) SASP tag is located at the very end of the SASP packet (which is always the case for SASP as described in :ref:`sasp` document), and (b) all protocols below SASP add only headers, and not trailers (which is usually, but not strictly necessarily, the case for DLP protocols).

If the size of encrypted-post-unique-pre-SCRAMBLING-Data is always zero, it means that there is no need to send *unique-block-offset* over the wire, as it can always be calculated on receiving side. Therefore, Default SCRAMBLING-Header Schema is defined as follows:


**\| Forced-Padding-Flag-And-Scrambled-Size \| Optional-Forced-Padding-Size \| unencrypted-pre-SCRAMBLING-Data \|**

where Forced-Padding-Flag-And-Scrambled-Size is an Encoded-Unsigned-Int<max=2> field, which acts as a substrate for bitfields Forced-Padding-Flag (takes bit [0]), and Scrambled-Size (takes bits [1..]), and Optional-Forced-Padding-Size is an Encoded-Unsigned-Int<max=2> field which is present only if Forced-Padding-Flag is equal to 1.


DESCRAMBLING
------------

Processing of a SCRAMBLED packet ("DESCRAMBLING") is performed in reverse order compared to SCRAMBLING procedure. 

"Streamed" SCRAMBLING
---------------------

There are cases, where SCRAMBLED data is intended to be sent over stream (such as TCP stream), other than in individual datagrams. In such cases, "Streamed" SCRAMBLING may be used. "Streamed" SCRAMBLING differs from SCRAMBLING procedure above in the following details:

* when SCRAMBLING-Header is formed, it includes Whole-Packet-Size (as the very first field), followed by all the fields specified in SCRAMBLING procedure above.

where Whole-Packet-Size is an Encoded-Unsigned-Int<max=2> field, representing the whole packet size (excluding forced-padding if any).

As even Whole-Packet-Size is scrambled, the whole stream looks as a white noise (NB: some information can be still extracted by attacker from timing and division of the stream into packets). 

To ensure proper error recovery, receiving side of "Streamed"-SCRAMBLED stream MUST forcibly break an underlying stream (such as TCP connection) as soon as any of the de-SCRAMBLING operations for packets received over this underlying connection fail (this includes size field exceeding it's "max=" size).


TODO: forced-padding (incl. random-size padding)

