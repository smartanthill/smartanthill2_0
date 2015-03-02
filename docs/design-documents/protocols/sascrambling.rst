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

SmartAnthill SCRAMBLING procedure and SmartAnthill Random Generation
====================================================================

:Version:   v0.4.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SmartAnthill SCRAMBLING procedure aims to provide some extra protection when data is transmitted in open (in particular, over wireless or over the Internet). **SCRAMBLING procedure does not provide security guarantees** in a strict sense, but might hide certain details (such as source/destination addresses) and does help against certain classes of DoS attacks. Because of the lack of security guarantees, SCRAMBLING procedure SHOULD NOT be used as a sole encryption protocol (using it over SASP is fine).

SCRAMBLING procedure requires both sides to share two secret Speck-96 keys. **SCRAMBLING keys MUST be separate and independent from any other key in the system, in particular, from SASP keys.**

SCRAMBLING procedure is intended to be used as the outermost packet wrapper which is possible for an underlying protocol. Within SmartAnthill Protocol Stack, SCRAMBLING procedure is OPTIONALLY used by SAoIP protocol as described in :ref:`saoip` document. In addition, SADLP-\* protocols, especially those working over wireless L1 protocols, SHOULD use SCRAMBLING procedure to hide as much information as possible. 

.. contents::

Environment
-----------

SCRAMBLING procedure is a procedure of taking an input packet of arbitrary size, and producing a "scrambled" packet. It is used both by SAoIP and some of SADLP-\*.

SCRAMBLING procedure requires both sides to share two secret Speck-96 keys. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

For SCRAMBLING procedure to be efficient (in secure sense), caller SHOULD guarantee that there is a 12-byte block within input packet, where such block is at least statistically unique. Offset to such a block within the packet is an input *unique-block-offset* parameter for SCRAMBLING procedure. 

SmartAnthill Random Number Generation
-------------------------------------

All random numbers which are used for SmartAnthill SASP protocol and SCRAMBLING procedure, MUST be generated in the manner described below.

Poor-Man's PRNG
^^^^^^^^^^^^^^^

Each device with Poor-Man's PRNG, has it's own Speck-96, Speck-128, or AES-128 secret key (this key MUST NOT be stored outside of the device), and additionally keeps a counter. This counter MUST be kept in a way which guarantees that the same value of the counter is never reused; this includes both having counter of sufficient size, and proper commits to persistent storage to avoid re-use of the counter in case of accidental device reboot. As for commits to persistent storage - two such implementations are discussed in :ref:`sasp` document, in 'Implementation Details' section, with respect to storing nonces.

Then, Poor-Man's PRNG simply encrypts current value of the counter with Speck-96, Speck-128, or AES-128, increments counter (see note above about guarantees of no-reuse), and returns encrypted value of the counter as next 16 bytes of the random output.

Devices without crypto-safe RNG
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Resource-constrained SmartAnthill Devices which don't have their own crypto-safe RNG, SHOULD use Poor-Man's PRNG. 

However, such devices with only Poor-Man's PRNG, MUST NOT generate keys.

Devices with crypto-safe RNG
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Even if the system where the SmartAnthill stack is running has a supposedly crypto-safe RNG (such as built-in crypto-safe /dev/urandom), SmartAnthill implementations still SHOULD employ Poor-Man's PRNG (as described above) in addition to system-provided crypto-safe PRNG. In such cases, each byte of SmartAnthill RNG (which is provided to the rest of SmartAnthill) SHOULD be a XOR of 1 byte of system-provided crypto-safe PRNG, and 1 byte of Poor-Man's PRNG. 

The same procedure SHOULD also be used for generating random data which is used for SmartAnthill key generation. 

*Rationale. This approach allows to reduce the impact of catastrophic failures of the system-provided crypto-safe PRNG (for example, it would mitigate effects of the Debian RNG disaster very significantly).*

TODO: define key generation for Poor-Man's PRNG

SCRAMBLING procedure
--------------------

Input
^^^^^

Input of SCRAMBLING procedure is a pre-SCRAMBLING packet, and *unique-block-offset* offset. pre-SCRAMBLING packet can be considered as follows:

**\| pre-unique-pre-SCRAMBLING-Data \| unique-block \| post-unique-pre-SCRAMBLING-Data \|**

where unique-block is always 12 bytes in size, and it's offset from the beginning is specified by *unique-block-offset* parameter, and both pre-unique-pre-SCRAMBLING-Data and post-unique-pre-SCRAMBLING-Data can have 0 size.

If *unique-block-offset+12* goes beyond the end of pre-SCRAMBLING-Data, SCRAMBLING procedure adjusts it to *size(pre_SCRAMBLING_Data)-12*.

If pre-SCRAMBLING-Data has size < 12, it is padded to 12 bytes with random data to form unique-block, and Unique-Block-Padded field (described below) is set to size of required padding.

Procedure
^^^^^^^^^

SCRAMBLING procedure works as follows:

1. Form pre-encrypted packet which has the following format:

**\| Salt \| unique-block \| Padding-Size \| Padding \| Unique-Block-Padded \| Unique-Block-Offset \| pre-unique-pre-SCRAMBLING-Data \| post-unique-pre-SCRAMBLING-Data \|**

where Salt is a 12-byte random field (NB: endianness of Salt doesn't matter), Padding-Size is Encoded-Unsigned-Int<max=2>, Padding is optional padding (0 to 15 bytes unless forced-padding is used), which has size of Padding-Size, Unique-Block-Padded is a 1-byte field (with maximum value of 12) which indicates that unique-block itself has been padded, and size of this padding (which happens only if size of pre-SCRAMBLING-Data is < 12). Unique-Block-Offset is Encoded-Unsigned-Int<max=2> (equal to *unique-block-offset* parameter*). Both Salt and Padding SHOULD be cryptographically random (for example, generated by Fortuna RNG) whenever feasible; if this is not feasible, Poor-Man's PRNG (described above) is acceptable. NB: placing Padding as early in the pre-encrypted packet is intentional, to inject more randomicity into the CBC as early as possible. NB2: Salt is merely an additional precaution measure to guarantee statistical uniqueness. 

The size of Padding is calculated to ensure that pre-encrypted packet has size of 16\*k bytes where k is integer.

2. Encrypt pre-encrypted packet with the secret key, using Speck-96 (with 96-bit block size) in CBC mode, first Speck-96 key, and using random 96-bit IV (which MUST be fully random, in particular, different from Salt). CBC mode, combined with statistical-uniqueness requirement for unique-block, ensures that SCRAMBLED data is indistinguishable from white noise for a potential attacker.

3. Calculate CBC-MAC of the encrypted packet, using Speck-96 (with 96-bit block size) and second Speck-96 key.

4. Form output packet as follows:

**\| Encrypted-Packet \| CBC-MAC \|**

where CBC-MAC is 12-byte CBC-MAC field.

DESCRAMBLING
------------

Processing of a SCRAMBLED packet ("DESCRAMBLING") is performed in reverse order compared to SCRAMBLING procedure. 

If CBC-MAC in the packet being descrambled, doesn't validate, then DESCRAMBLING procedure returns failire without any further processing of the packet. 

CBC decryption SHOULD be done with an arbitrary IV (for example, all-zero IV); this will lead to incorrect decryption of *Salt* field (which we don't care about anyway), but will keep all the other blocks intact.

"Streamed" SCRAMBLING
---------------------

There are cases, where SCRAMBLED data is intended to be sent over stream (such as TCP stream), other than in individual datagrams. In such cases, "Streamed" SCRAMBLING may be used. "Streamed" SCRAMBLING (which takes pre-SCRAMBLED as it's input) is defined as follows:

* pre-SCRAMBLED packet is SCRAMBLED as described in SCRAMBLING procedure above, forming *SCRAMBLED-Data packet*
* size of *SCRAMBLED-Data packet* is calculated, and encoded as Encoded-Size<max=2>
* this size itself is SCRAMBLED (again, as described in SCRAMBLING procedure above), forming *SCRAMBLED-Size*. Note that this *SCRAMBLED-Size* data always has size of two 12-byte blocks.
* *Streamed-SCRAMBLING pseudo-packet* is formed as follows:

**\| SCRAMBLED-Size \| SCRAMBLED-Data packet \|**

This *Streamed-SCRAMBLING pseudo-packet* can be sent over the stream. As even packet size is scrambled, the whole stream looks as a white noise (NB: some information can be still extracted by attacker from timing and division of the stream into packets). 

"Streamed" DESCRAMBLING
^^^^^^^^^^^^^^^^^^^^^^^

To decode "Streamed"-SCRAMBLED stream, the procedure looks as follows:

* take first two 12-byte blocks of the stream
* we know that these two blocks represent SCRAMBLED-Size field
* this SCRAMBLED-Size field is de-SCRAMBLED, and we obtain size of the *SCRAMBLED-Data Packet*
* now, we can de-SCRAMBLE *SCRAMBLED-Data Packet*
* repeat the procedure from the very beginning

To ensure proper error recovery, receiving side of "Streamed"-SCRAMBLED stream MUST forcibly break an underlying stream (such as TCP connection) as soon as any of the de-SCRAMBLING operations for packets received over this underlying connection fail. 


VARIATIONS
----------

If resources of the SmartAnthill Device are limited (or device is performing secure function), SmartAnthill Device and SmartAnthill Controller MAY agree on using a different flavour of Speck algorithm (such an agreement SHOULD happen during "pairing" or "programming" as described in :ref:`saoverarch` document). However, in secure environments (for example, if at least one of SmartAnthill Devices on the same wireless bus is performing a security-related function) it is important to ensure that all devices produce the packets of the same length. 

Allowed combinations for Speck parameters and SCRAMBLING parameters are the following:

+--------------------+-------------------------+-----------+-------------------+-------------+
| Speck block size   | Speck key length        | Salt Size | Unique Block Size | Remarks     |
+====================+=========================+===========+===================+=============+
|128 bit             |128 bit                  | 16 bytes  | 16 bytes          | Improved    |
+--------------------+-------------------------+-----------+-------------------+-------------+
| 96 bit             | 96 bit                  | 12 bytes  | 12 bytes          | Default     |
+--------------------+-------------------------+-----------+-------------------+-------------+
| 48 bit             | 72 bit                  |  6 bytes  |  6 bytes          | Reduced     |
+--------------------+-------------------------+-----------+-------------------+-------------+
| 32 bit             | 64 bit                  |  4 bytes  |  4 bytes          | Minimal     |
+--------------------+-------------------------+-----------+-------------------+-------------+


TODO: forced-padding (incl. random-size padding)

