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

:Version:   v0.3.2

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SmartAnthill SCRAMBLING procedure aims to provide some extra protection when data is transmitted in open (in particular, over wireless or over the Internet). **SCRAMBLING procedure does not provide security guarantees** in a strict sense, but might hide certain details (such as source/destination addresses) and does help against certain classes of DoS attacks. Because of the lack of security guarantees, SCRAMBLING procedure SHOULD NOT be used as a sole encryption protocol (using it over SASP is fine).

SCRAMBLING procedure requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

SCRAMBLING procedure is intended to be used as the outermost packet wrapper which is possible for an underlying protocol. Within SmartAnthill Protocol Stack, SCRAMBLING procedure is OPTIONALLY used by SAoIP protocol as described in :ref:`saoip` document. In addition, SADLP-\* protocols, especially those working over wireless L1 protocols, SHOULD use SCRAMBLING procedure to hide as much information as possible. 

.. contents::

Environment
-----------

SCRAMBLING procedure is a procedure of taking an input packet of arbitrary size, and producing a "scrambled" packet. It is used both by SAoIP and some of SADLP-\*.

SCRAMBLING procedure requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

For SCRAMBLING procedure to be efficient (in secure sense), caller SHOULD guarantee that there is a 8-byte block within input packet, where such block is at least statistically unique. Offset to such a block within the packet is an input *unique-block-offset* parameter for SCRAMBLING procedure. 


DUMB-CHECKSUM
-------------

DUMB-CHECKSUM is not intended to provide strict consistency guarantees on the data sent. However, in case of a DoS attack, getting through DUMB-CHECKSUM without knowledge of the secret key, is difficult (chances of getting through at least for a dumb DoS attack are less than 10^-9).

DUMB-CHECKSUM is calculated as follows:

* pre-SCRAMBLING data is divided into 32-bit blocks. If the last block is not full, it is zero-padded.
* initial CHK is set to 0
* for each 32-bit block, CHK is XORed with the block itself.
* resulting value is DUMB-CHECKSUM

It should be noted that due to the manner how DUMB-CHECKSUM is constructed, it can be considered as four independent 8-bit checksums (C0 is XOR of the bytes #0, #4, #8, ...,  C1 is XOR of the bytes #1, #5, #9, ..., C2 is XOR of the bytes #2, #6, #10, ..., and C3 is XOR of the bytes #3, #7, #11, ...). These four bytes MUST be written in the same order as their original bytes, i.e. as C0 C1 C2 C3. It ensures that DUMB-CHECKSUM is endian-agnostic (i.e. doesn't depend on the endianness).

POOR-MANS PRNG
--------------

In cases, when obtaining a cryptographically safe random numbers is not feasible (for example, on MPUs), the following "Poor Man's PRNG" MAY be used for padding. 

Each device with Poor-Man's PRNG, has it's own AES-128 secret key (this key MUST NOT be stored outside of the device), and additionally keeps a counter. This counter MUST be kept in a way which guarantees that the same value of the counter is never reused; this includes both having counter of sufficient size, and proper commits to persistent storage to avoid re-use of the counter in case of accidental device reboot. As for commits to persistent storage - two such implementations are discussed in :ref:`sasp` document, in 'Implementation Details' section, with respect to storing nonces.

Then, Poor-Man's PRNG simply encrypts current value of the counter with AES-128, increments counter (see note above about guarantees of no-reuse), and returns encrypted value of the counter as next 16 bytes of the random output.

TODO: Speck cipher instead of AES for both SCRAMBLING and Poor-Man's PRNG? (Speck has been reported to be about 3x faster than AES on MP430)

SCRAMBLING procedure
--------------------

Input
^^^^^

Input of SCRAMBLING procedure is a pre-SCRAMBLING packet, and *unique-block-offset* offset. pre-SCRAMBLING packet can be considered as follows:

**\| pre-unique-pre-SCRAMBLING-Data \| unique-block \| post-unique-pre-SCRAMBLING-Data \|**

where unique-block is always 8 bytes in size, and it's offset from the beginning is specified by *unique-block-offset* parameter, and both pre-unique-pre-SCRAMBLING-Data and post-unique-pre-SCRAMBLING-Data can have 0 size.

If *unique-block-offset+8* goes beyond the end of pre-SCRAMBLING-Data, SCRAMBLING procedure adjusts it to *size(pre_SCRAMBLING_Data)-8*.

TODO: pre-SCRAMBLING-Data < 8

Procedure
^^^^^^^^^

SCRAMBLING procedure works as follows:

1. Form pre-encrypted packet which has the following format:

**\| Salt \| unique-block \| Padding-Size \| Padding \| Dumb-Checksum \| Unique-Block-Offset \| pre-unique-pre-SCRAMBLING-Data \| post-unique-pre-SCRAMBLING-Data \|**

where Salt is an 8-byte random field (NB: endianness of Salt doesn't matter), Padding-Size is Encoded-Unsigned-Int<max=2>, Padding is optional padding (0 to 15 bytes unless forced-padding is used), which has size of Padding-Size. Unique-Block-Offset is Encoded-Unsigned-Int<max=2> (equal to *unique-block-offset* parameter*),  Dumb-Checksum is 4-byte DUMB-CHECKSUM of the pre-SCRAMBLING-Data (NB: DUMB-CHECKSUM as described above, is endianness-agnostic). Both Salt and Padding SHOULD be cryptographically random (for example, generated by Fortuna RNG) whenever feasible; if this is not feasible, Poor-Man's PRNG (described above) is acceptable. NB: placing Padding as early in the pre-encrypted packet is intentional, to inject more randomicity into the stream (formed at the next step) as early as possible. NB2: Salt is merely an additional precaution measure to guarantee statistical uniqueness. 

The size of Padding is calculated to ensure that pre-encrypted packet has size of 16\*k bytes where k is integer.

2. Form "XORed" packet as follows: 

   * split pre-encrypted packet into 16-byte blocks (all blocks are full, as pre-encrypted packet has already been padded to 16\*k bytes)
   * first block is left as is, second block is XOR-ed with the first one, third block is XORed with both first block and original (pre-XOR-ed) second block, fourth block is XORed with first block, original second block, and original third block, and so on

This "XORed" packet intends to propagate uniqueness (which is contained in the first block) over the whole message. Encryption of such "XORed" packet, combined with statistical-uniqueness requirement for unique-block, ensures that SCRAMBLED data is indistinguishable from white noise for a potential attacker.

3. Encrypt "XOR-ed" packet with the secret key, using AES-128 in CTR mode. 

3. Resulting encrypted packet is the output of SCRAMBLING procedure. TODO: 2nd post-encryption padding?

DESCRAMBLING
------------

Processing of a SCRAMBLED packet ("DESCRAMBLING") is performed in reverse order compared to SCRAMBLING procedure. If Dumb-Checksum in the packet being descrambled, doesn't match DUMB-CHECKSUM calculated as described above, then DESCRAMBLING procedure returns failire.

TODO: forced-padding (incl. random padding)

