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

:Version:   v0.3

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SmartAnthill SCRAMBLING procedure aims to provide some extra protection when data is transmitted in open (in particular, over wireless or over the Internet). **SCRAMBLING procedure does not provide security guarantees** in a strict sense, but might hide certain details (such as source/destination addresses) and does help against certain classes of DoS attacks. Because of the lack of security guarantees, SCRAMBLING procedure SHOULD NOT be used as a sole encryption protocol (using it over SASP is fine).

SCRAMBLING procedure requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

SCRAMBLING procedure is intended to be used as the outermost packet wrapper which is possible for an underlying protocol. Within SmartAnthill Protocol Stack, SCRAMBLING procedure is OPTIONALLY used by SAoIP protocol as described in :ref:`saoip` document. In addition, SADLP-\* protocols, especially those working over wireless L1 protocols, SHOULD use SCRAMBLING procedure to hide as much information as possible. 

.. contents::

Environment
-----------

SCRAMBLING procedure is a procedure of taking an input packet of arbitrary size, and producing a "scrambled" packet. It is used both by SAoIP and some of SADLP-\*.

SCRAMBLING procedure requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

For SCRAMBLING procedure to be efficient (in secure sense), caller SHOULD guarantee that there is a 12-byte block within input packet, where such block is at least statistically unique. Offset to such a block within the packet is an input *unique-block-offset* parameter for SCRAMBLING procedure. 


DUMB-CHECKSUM
-------------

DUMB-CHECKSUM is not intended to provide strict consistency guarantees on the data sent. However, in case of a DoS attack, getting through DUMB-CHECKSUM without knowledge of the secret key, is difficult (chances of getting through at least for a dumb DoS attack are less than 10^-9).

DUMB-CHECKSUM is calculated as follows:

* pre-SCRAMBLING data is divided into 32-bit blocks. If the last block is not full, it is zero-padded.
* initial CHK is set to 0
* for each 32-bit block, CHK is XORed with the block itself.
* resulting value is DUMB-CHECKSUM

It should be noted that due to the manner how DUMB-CHECKSUM is constructed, it can be considered as four independent 8-bit checksums (C0 is XOR of the bytes #0, #4, #8, ...,  C1 is XOR of the bytes #1, #5, #9, ..., C2 is XOR of the bytes #2, #6, #10, ..., and C3 is XOR of the bytes #3, #7, #11, ...). These four bytes MUST be written in the same order as their original bytes, i.e. as C0 C1 C2 C3. It ensures that DUMB-CHECKSUM is endian-agnostic (i.e. doesn't depend on the endianness).

SCRAMBLING procedure
--------------------

Input
^^^^^

Input of SCRAMBLING procedure is a pre-SCRAMBLING packet, and *unique-block-offset* offset. pre-SCRAMBLING packet can be considered as follows:

**\| pre-unique-pre-SCRAMBLING-Data \| unique-block \| post-unique-pre-SCRAMBLING-Data \|**

where unique-block is always 12 bytes in size, and it's offset from the beginning is specified by *unique-block-offset* parameter, and both pre-unique-pre-SCRAMBLING-Data and post-unique-pre-SCRAMBLING-Data can have 0 size.

If *unique-block-offset+12* goes beyond the end of pre-SCRAMBLING-Data, SCRAMBLING procedure adjusts it to *size(pre_SCRAMBLING_Data)-12*.

TODO: pre-SCRAMBLING-Data < 12

Procedure
^^^^^^^^^

SCRAMBLING procedure works as follows:

1. Form pre-encrypted packet which has the following format:

**\| Salt \| unique-block \| Padding-Size \| Padding \| Unique-Block-Offset \| pre-unique-pre-SCRAMBLING-Data \| post-unique-pre-SCRAMBLING-Data \| Dumb-Checksum \|**

where Salt is a 4-byte random field (NB: endianness of Salt doesn't matter), Padding-Size is Encoded-Unsigned-Int<max=2>, Padding is optional padding (0 to 15 bytes unless forced-padding is used), which has size of Padding-Size. Unique-Block-Offset is Encoded-Unsigned-Int<max=2> (equal to *unique-block-offset* parameter*),  Dumb-Checksum is 4-byte DUMB-CHECKSUM of the pre-SCRAMBLING-Data (NB: DUMB-CHECKSUM as described above, is endianness-agnostic). Both Salt and Padding SHOULD be cryptographically random whenever feasible; however, even simple randomicity from triial linear congruential 32-bit pseudo-RNG is acceptable for resource-restricted devices. NB: placing Padding as early in the pre-encrypted packet is intentional, to inject more randomicity into the CBC as early as possible. NB2: Salt is merely an additional precaution measure. 

The size of Padding is calculated to ensure that pre-encrypted packet has size of 16\*k bytes where k is integer.

2. Encrypt pre-encrypted packet with the secret key, using AES-128 in CBC mode. CBC mode, combined with statistical-uniqueness requirement for unique-block, ensures that SCRAMBLED data is indistinguishable from white noise for a potential attacker.

3. Resulting encrypted packet is the output of SCRAMBLING procedure. TODO: 2nd post-encryption padding?

DESCRAMBLING
------------

Processing of a SCRAMBLED packet ("DESCRAMBLING") is performed in reverse order compared to SCRAMBLING procedure. If Dumb-Checksum in the packet being descrambled, doesn't match DUMB-CHECKSUM calculated as described above, then DESCRAMBLING procedure returns failire.

TODO: forced-padding (incl. random padding)

