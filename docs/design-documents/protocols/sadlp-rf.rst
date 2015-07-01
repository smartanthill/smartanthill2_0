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

.. _sadlp-rf:

SmartAnthill DLP for RF (SADLP-RF)
==================================

:Version:   v0.3

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SADLP-RF provides L2 datalink over simple Radio-Frequency channels which have only an ability to send/receive packets over RF without any addressing. For more complicated RF communications (such as IEEE 802.15.4), different SADLP-\* protocols (such as SADLP-802.15.4 described in :ref:`sadlp-802-15-4`) need to be used.

SADLP-RF PHY Level
------------------

Frequencies: TODO (with frequency shifts)

Modulation: 2FSK (a.k.a. FSK without further specialization, and BFSK), or GFSK (2FSK and GFSK are generally compatible), with frequency shifts specified above.

Baud rate: 9600 baud (TODO: negotiate?).

SADLP-RF Design
---------------

Assumptions:

* We're assuming to operate in a noisy environment. Hence, we need to use forward error correction.
* error correction level is to be specified by an upper protocol layer for each packet separately (for example, retransmits may use higher error correction levels)
* We don't have enough resources to run sophisticated error-correction mechanisms such as Reed-Solomon, Viterbi, etc.
* Transmissions are rare, hence beacons and frequency hopping are not used
* upper protocol layer may have some use for packets where only a header is provided; hence upper layer provides it's header and it's payload separately

Non-paired Addressing for RF Buses
----------------------------------

Each RF frequency channel on a Device represents a "wireless bus" in terms of SAMP. For "intra-bus address" as a part "non-paired addressing" (as defined in :ref:`samp`), RF Devices MUST use randomly generated 64-bit ID. 

If Device uses hardware-assisted Fortuna PRNG (as described in :ref:`sarng` document), Device MUST complete Phase 1 of "Entropy Gathering Procedure" (as described in :ref:`sapairing` document) to initialize Fortuna PRNG *before* generating this 64-bit ID. Then, Device should proceed to Phase 2 (providing Device ID), and Phase 3 (entropy gathering for key generation purposes), as described in :ref:`sapairing` document.

Device Discovery and Pairing
----------------------------

For Devices with OtA Pairing (as described in :ref:`sapairing`), "Device Discovery" procedure described in :ref:`samp` document is used, with the following clarifications:

* SAMP "channel scan" for SADLP-RF is performed as follows:

  - "candidate channel" list consists of all the channels allowed in target area
  - for each of candidate channels:

    + the first packet as described in SAMP "Device Discovery" procedure is sent by Device
    + if a reply is received indicating that Root is ready to proceed with "pairing" - "pairing" is continued over this channel
      
      - if "pairing" fails, then the next available "candidate channel" is processed. 
      - to handle the situation when "pairing" succeeds, but Device is connected to wrong Central Controller - Device MUST (a) provide a visual indication that it is "paired", (b) provide a way (such as jumper or button) allowing to drop current "pairing" and continue processing "candidate channels". In the latter case, Device MUST process remaining candidate channels before re-scanning.
 
    + if a reply is received with ERROR-CODE = ERROR_NOT_AWAITING_PAIRING, or if there is no reply within 500 msec, the procedure is repeated for the next candidate channel

  - if the list of "candidate channels" is exhausted without "pairing", the whole "channel scan" is repeated (indefinitely, or with a 5-or-more-minute limit - if the latter, then "not scanning anymore" state MUST be indicated on the Device itself - TODO acceptable ways of doing it, and the scanning MUST be resumed if user initiates "re-pairing" on the Device), starting from an "active scan" as described above


SADLP-RF Packet
---------------

SADLP-RF packet has the following format:

**\| ENCODING-TYPE \| REPEATED-ENCODING-TYPE \| SADLP-RF-DATA \|**

where ENCODING-TYPE and REPEATED-ENCODING-TYPE are 1-byte fields (transmitted identical), with possible values being NO-CORRECTION, HAMMING-32-CORRECTION, and HAMMING-32-2D-CORRECTION.

NO-CORRECTION
^^^^^^^^^^^^^

For NO-CORRECTION packets, SADLP-RF-DATA has the following format:

**\| UPPER-LAYER-HEADER-LENGTH \| UPPER-LAYER-HEADER \| UPPER-LAYER-HEADER-CHECKSUM \| UPPER-LAYER-PAYLOAD-LENGTH \| UPPER-LAYER-PAYLOAD \| UPPER-LAYER-HEADER-AND-PAYLOAD-CHECKSUM \|**

where UPPER-LAYER-HEADER-LENGTH is an Encoded-Unsigned-Int<max=2> field specifying size of UPPER-LAYER-HEADER, UPPER-LAYER-HEADER-CHECKSUM is a 2-byte field containing SACHECKSUM-16 of UPPER-LAYER-HEADER, UPPER-LAYER-PAYLOAD-LENGTH is an Encoded-Unsigned-Int<max=2> field specifying size of UPPER-LAYER-PAYLOAD, and UPPER-LAYER-HEADER-AND-PAYLOAD CHECKSUM is a 2-byte field containing SACHECKSUM-16 of UPPER-LAYER-HEADER concatenated with UPPER-LAYER-PAYLOAD.

HAMMING-32-CORRECTION
^^^^^^^^^^^^^^^^^^^^^

For HAMMING-32-CORRECTION packets, SADLP-RF-DATA is **\| UPPER-LAYER-HEADER-WITH-HAMMING-32 \| UPPER-LAYER-PAYLOAD-WITH-HAMMING-32 \|**

where UPPER-LAYER-HEADER-WITH-HAMMING-32 and UPPER-LAYER-PAYLOAD each is represented by a sequence of 4-byte HAMM32 error-correctable blocks (out of each 4-byte block 26 bits are reconstructed). Each HAMM32 block is a 31-bit Hamming block (as described in https://en.wikipedia.org/wiki/Hamming_code), with prepended total block parity bit p0 (making HAMM32 a 32-bit SECDED code).

To produce UPPER-LAYER-HEADER-WITH-HAMMING-32 from UPPER-LAYER-HEADER, the following procedure is used:

* If UPPER-LAYER-HEADER have size which is not a multiple of 26-bit, it is padded with random data (using non-key random stream as specified in :ref:`sarng`). 
* resulting bit sequence is split into 26-bit chunks, and each 26-bit chunk is converted into a 32-bit HAMM32 block
* UPPER-LAYER-HEADER-WITH-HAMMING-32 is a sequence of resulting 32-bit HAMM32 blocks

To produce UPPER-LAYER-PAYLOAD-WITH-HAMMING-32 from UPPER-LAYER-PAYLOAD, the procedure is exactly the same.

UPPER-LAYER-HEADER has the following format:

**\| UPPER-LAYER-HEADER-LENGTH \| UPPER-LAYER-HEADER \| UPPER-LAYER-HEADER-CHECKSUM \|**

where UPPER-LAYER-HEADER-LENGTH is an Encoded-Unsigned-Int<max=2> field specifying size of UPPER-LAYER-HEADER, and UPPER-LAYER-HEADER-CHECKSUM is a 2-byte field containing SACHECKSUM-16  of UPPER-LAYER-HEADER.

UPPER-LAYER-PAYLOAD has the following format:

**\| UPPER-LAYER-PAYLOAD-LENGTH \| UPPER-LAYER-PAYLOAD \| UPPER-LAYER-HEADER-AND-PAYLOAD-CHECKSUM \|**

where UPPER-LAYER-PAYLOAD-LENGTH is an Encoded-Unsigned-Int<max=2> field specifying size of UPPER-LAYER-PAYLOAD, and UPPER-LAYER-HEADER-AND-PAYLOAD CHECKSUM is a 2-byte field containing SAHECKSUM-16 of UPPER-LAYER-HEADER concatenated with UPPER-LAYER-PAYLOAD.

HAMMING-32-2D-CORRECTION
^^^^^^^^^^^^^^^^^^^^^^^^

HAMMING-32-2D-CORRECTION is similar to HAMMING-32-CORRECTION, with the following differences.

Both UPPER-LAYER-HEADER-WITH-HAMMING-32 and UPPER-LAYER-PAYLOAD-WITH-HAMMING-32 have 26 additional Hamming checksums added at the end; each Hamming checksum #i consists of N parity bits of Hamming code, calculated over all bits #i in 26-bit data bits within HAMM32 blocks. Number N is a number of Hamming bits necessary to provide error correction for NN=NUMBER-OF-HAMM32-BLOCKS. Hamming checksums are encoded as a bitstream, without intermediate padding, but padded at the end to a byte boundary with random (non-key-stream) data.

For example, if original block is 50 bytes long, then it will be split into 16 26-bit blocks, which will be encoded as 16 HAMM32 blocks; then, for HAMMING-32-2D-CORRECTION, additional 26 Hamming checksums (5 bits each, as for NN=16 N=5) will be added. Therefore, original 50 bytes will be encoded as 4*16+17=81 byte (62% overhead).

