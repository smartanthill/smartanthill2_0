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

.. _sascp:

SmartAnthill Scrambling Protocol (SAScP) and SCRAMBLING procedure
=================================================================

:Version:   v0.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SAScP (SmartAnthill Scrambling Protocol) aims to provide some extra protection when data is transmitted in open (over wireless). **SAScP does not provide security guarantees** in a strict sense, but might hide certain details (such as destination addresses) and does help against certain classes of DoS attacks. Because of the lack of security guarantees, SAScP SHOULD NOT be used as a sole encryption protocol (using it over SASP is fine).

SAScP requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

Within SmartAnthill Protocol Stack, SAScP resides below SASP on the way between SmartAnthill Router and SmartAnthill Simple Device. SAScP as such is not used for communicating with SmartAnthill IP-Enabled Devices; however, SCRAMBLING procedure described in present document, MAY be used by SAoIP protocol as described in :ref:`saoip` document. 

.. contents::

Reverse Parsing and Reverse-Encoded-Int
---------------------------------------

To comply with requirements of SCRAMBLING procedure which are described below, certain headers in SCRAMBLING and associated protocols are located at the end of the packet. As a result, parsing should be performed starting from the end of the packet. To facilitate such a 'reverse parsing', 'Reverse-Encoded-Int' encoding is used; Reverse-Encoded-Int<max=n> encoding is identical to Encoded-Int<max=n> encoding as defined in :ref:`saprotostack` document, except that all the bytes are written (and parsed) in the reverse order.


SCRAMBLING
----------

SCRAMBLING procedure is a procedure of taking an input packet of arbitrary size, and producing a "scrambled" packet. It is used both by SAScP and by SAoIP.

SCRAMBLING procedure requires both sides to share a secret AES-128 key. **SCRAMBLING key MUST be independent from any other key in the system, in particular, from SASP key.**

For SCRAMBLING procedure to be efficient, it SHOULD ensure that all the first-16-byte-blocks of pre-SCRAMBLING data, are at least statistically unique. For existing SASP packets, it can be guaranteed as long as within first 16 bytes of pre-SCRAMBLING packet, there are at least 7 bytes of the SASP. To ensure that this always stands, SAoIP uses unusual packet structure with headers at the end; for SAScP, header is fixed-size and is small enough to ensure that the guarantee stands.

DUMB-CHECKSUM
^^^^^^^^^^^^^

DUMB-CHECKSUM is not intended to provide strict consistency guarantees on the data sent. However, in case of a DoS attack, getting through DUMB-CHECKSUM without knowledge of the secret key, is difficult (chances of getting through at least for a dumb DoS attack are less than 10^-9).

DUMB-CHECKSUM is calculated as follows:

* pre-SCRAMBLING data is divided into 32-bit blocks. If the last block is not full, it is zero-padded.
* initial CHK is set to 0
* for each 32-bit block, CHK is XORed with the block itself.
* resulting value is DUMB-CHECKSUM

It should be noted that due to the manner how DUMB-CHECKSUM is constructed, it can be considered as four independent 8-bit checksums (C0 is XOR of the bytes #0, #4, #8, ...,  C1 is XOR of the bytes #1, #5, #9, ..., C2 is XOR of the bytes #2, #6, #10, ..., and C3 is XOR of the bytes #3, #7, #11, ...). These four bytes MUST be written in the same order as their original bytes, i.e. as C0 C1 C2 C3. It ensures that DUMB-CHECKSUM is endian-agnostic (i.e. doesn't depend on the endianness).

SCRAMBLING procedure
^^^^^^^^^^^^^^^^^^^^

Input of SCRAMBLING procedure is a pre-SCRAMBLING packet. SCRAMBLING procedure works as follows:

1. Form pre-encrypted packet which has the following format:

**\| pre-SCRAMBLING-Data \| Dumb-Checksum \| Padding \| Padding-Size \|**

where Dumb-Checksum is 4-byte DUMB-CHECKSUM of the pre-SCRAMBLING-Data, Padding is optional padding (0 to 15 bytes unless forced-padding is used), Padding-Size is a Reverse-Encoded-Int<max=2>, which specifies amount of padding in use (value of Padding-Size includes both size of Padding and size of Padding-Size itself). Padding-Size is at least 1 byte long, and has a minimum value of 1. Padding SHOULD be cryptographically random. TODO: checksum?

The size of Padding is calculated to ensure that pre-encrypted packet has size of 16*k bytes.


2. Encrypt pre-encrypted packet with the secret key, using AES-128 in CBC mode. CBC mode, combined with statistical-uniqueness requirement for 1st block, ensures that SCRAMBLED data is indistinguishable from white noise for a potential attacker.

3. Resulting encrypted packet is the output of SCRAMBLING procedure. TODO: 2nd post-encryption padding?

DESCRAMBLING
^^^^^^^^^^^^

Processing of a SCRAMBLED packet ("DESCRAMBLING") is performed in reverse order compared to SCRAMBLING procedure. If Dumb-Checksum in the packet being descrambled, doesn't match DUMB-CHECKSUM calculated as described above, then DESCRAMBLING procedure returns failire.

TODO: forced-padding (incl. random padding)

SAScP
-----

SAScP stands for *SmartAnthill Scrambling Protocol*. It relies heavily on the SCRAMBLING procedure above.

First, SAScP produces an intermediate SAScP packet:

**\| Key-ID \| SAScP-Payload \|**

where Key-ID is an Encoded-Int<max=4> (and normally has meaning of an identifier of the key to be used by SASP on receiving side of communication). 

Then, SAScP applies SCRAMBLING procedure to the intermediate packet above, to obtain a SAScP packet. This scrambled SAScP packet is ready to be sent over the unprotected channel.

SCRAMBLING being optional
^^^^^^^^^^^^^^^^^^^^^^^^^

In some cases (for example, if SmartAnthill Simple Device is not security-critical AND is implemented on a very low-performance hardware, SmartAnthill Router and SmartAnthill Simple Device MAY agree on using SAScP without applying SCRAMBLING procedure. Such an agreement is normally done during programming and/or "pairing" phase of SmartAnthill Device Life Cycle (see :ref:`saoverarch` document for details), and is beyond the scope of present document. 

Formally, within SmartAnthill Protocol Stack omitting SCRAMBLING doesn't affect any security guarantees (as such guarantees are provided by SASP, which is not optional). However, as SCRAMBLING provides some benefits at a very low cost, by default SCRAMBLING procedure SHOULD be applied to all communications unless and until it is proven to be detrimental. 

