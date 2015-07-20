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

:Version:   v0.4.5

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SADLP-RF provides L2 datalink over simple Radio-Frequency channels which have only an ability to send/receive packets over RF without any addressing. For more complicated RF communications (such as IEEE 802.15.4), different SADLP-\* protocols (such as SADLP-802.15.4 described in :ref:`sadlp-802-15-4`) need to be used.

SADLP-RF Design
---------------

Assumptions:

* We're assuming to operate in a noisy environment. Hence, we need to use forward error correction.
* error correction level is to be specified by an upper protocol layer for each packet separately (for example, retransmits may use higher error correction levels)
* We don't have enough resources to run sophisticated error-correction mechanisms such as Reed-Solomon, Viterbi, etc.
* Transmissions are rare, hence beacons and frequency hopping are not used
* upper protocol layer may have some use for packets where only a header is correct; hence, packets with only first portion of the packet being correctable, SHOULD still be passed to upper protocol layer (non-correctable "tail" of the packet MAY be truncated)

SADLP-RF PHY Level
------------------

Frequencies: TODO (with frequency shifts)

Modulation: 2FSK (a.k.a. FSK without further specialization, and BFSK), or GFSK (2FSK and GFSK are generally compatible), with frequency shifts specified above.

Tau (minimum period with the same frequency during FSK modulation): 1/9600 sec. *NB: this may or may not correspond to 9600 baud transfer rate.* (TODO: rate negotiation?)

Line code: preamble (at least two 0xAA (TODO:check if it is really 0xAA or 0x55) symbols), followed by 0x2DD4 sync word, followed by "raw" SADLP-RF Packet as described below. 

SADLP-RF Packets, SCRAMBLING, and Line Codes
--------------------------------------------

FSK modulation used by SADLP-RF, does not require AC/DC balance. However, it requires to have at least one edge per N*tau (to keep synchronization). For example, for a popular RFM69 module, it is recommended to have at least one edge per 16*tau. SADLP-RF approach in this field is two fold: 

* first, there are strict guarantees on absolute minimum number of edges; however, these absolute-minimum guarantees MAY be lower than recommended 16 bytes. 
* second, SADLP-RF protocol uses "Salted-SCRAMBLING" procedure as defined in :ref:`sascrambling`. If by any chance Salted-SCRAMBLING does violate 16*tau requirement and the packet is lost (which is extremely unlikely to start with), "Salt" will be changed on the next retransmit and all the bits will be reshuffled, which leads to very-low probability of exceeding 16*tau for the retransmit. 

Statistical data (TODO: double-check): 

+--------------------------+-----------------------------------------------------+
| Run length               | Probability to occur in 2600-bit (325-byte) packet  |
+==========================+=====================================================+
| 17                       | 1.22%                                               |
+--------------------------+-----------------------------------------------------+
| 18                       | 0.61%                                               |
+--------------------------+-----------------------------------------------------+
| 19                       | 0.27%                                               |
+--------------------------+-----------------------------------------------------+
| 20                       | 0.13%                                               |
+--------------------------+-----------------------------------------------------+
| 21                       | 0.07%                                               |
+--------------------------+-----------------------------------------------------+
| 22                       | 0.03%                                               |
+--------------------------+-----------------------------------------------------+
| 23                       | 0.018%                                              |
+--------------------------+-----------------------------------------------------+
| 24                       | 0.010%                                              |
+--------------------------+-----------------------------------------------------+
| 25                       | 0.003%                                              |
+--------------------------+-----------------------------------------------------+
| 26                       | 0.002%                                              |
+--------------------------+-----------------------------------------------------+
| 27                       | 0.001%                                              |
+--------------------------+-----------------------------------------------------+
| 28+                      | 0.0005%                                             |
+--------------------------+-----------------------------------------------------+

As run-length of 17 is very unlikely to be fatal, and as probability of longer run-lengths is decreased exponentially, we hope that described statistical approach will be acceptable in practice.

As a result, SADLP-RF does not need any additional line codes, and SADLP-RF Packets MUST be transmitted directly over FSK (after preamble and sync word, as described above).

SADLP-RF MTU Limits
-------------------

For RF, too long packets MAY increase chances of the packet being incorrect; this applies (though to the less extent) to the error-corrected packets.

*NB: numbers below are EXTREMELY preliminary, and are subject to change based on real-world experiments*

For ENCODING-TYPE=PLAIN16
^^^^^^^^^^^^^^^^^^^^^^^^^

Hard Limit: 128 bytes.
Soft Limit: 64 bytes.

For ENCODING-TYPE=HAMMING-32-CORRECTION
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hard Limit: 256 bytes.
Soft Limit: 128 bytes.

For ENCODING-TYPE=HAMMING-32-2D-CORRECTION
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hard Limit: 512 bytes.
Soft Limit: 256 bytes.


Non-paired Addressing for RF Buses
----------------------------------

Each RF frequency channel on a Device represents a "wireless bus" in terms of SAMP. For "intra-bus address" as a part "non-paired addressing" (as defined in :ref:`samp`), RF Devices MUST use randomly generated 64-bit ID. 

If Device uses hardware-assisted Fortuna PRNG (as described in :ref:`sarng` document), Device MUST complete Phase 1 of "Entropy Gathering Procedure" (as described in :ref:`sapairing` document) to initialize Fortuna PRNG *before* generating this 64-bit ID. Then, Device should proceed to Phase 2 (providing Device ID), and Phase 3 (entropy gathering for key generation purposes), as described in :ref:`sapairing` document.

PHY-Data-Request and PHY-Data-Response
--------------------------------------

As described in :ref:`samp` document, SACCP PHY-AND-ROUTING-DATA packets support PHY-Data-Request and PHY-Data-Response packets. For SADLP-RF, they're used as described below.

ID-OF-SADLP for SADLP-RF
^^^^^^^^^^^^^^^^^^^^^^^^

For SADLP-RF, ID-OF-SADLP is 0x0.

PHY-Data Packets for SADLP-RF
-----------------------------
SADLP-RF uses the following PHY-Data Packets:

Fine-Tune-Best-Frequency, going over PHY-Data-Response (sic!) and having SADLP-DEPENDENT-PAYLOAD of: **\| FREQUENCY-SCHEMA \| FREQUENCY \| FREQUENCY-WEIGHT \| FREQUENCY2 \| FREQUENCY-WEIGHT \| ... \|**
where FREQUENCY-SCHEMA is an Encoded-Unsigned-Int<max=1> (currently only LINEAR schema is supported), FREQUENCY is an Encoded-Unsigned-Int<max=2> field, FREQUENCY-WEIGHT is an Encoded-Unsigned-Int<max=2>.

Fine-Tune-Best-Frequency-Reply, going over PHY-Data-Request (sic!) and having SADLP-DEPENDENT-PAYLOAD of: **\| FREQUENCY \|**
where FREQUENCY is an Encoded-Unsigned-Int<max=2> field.

On receiving Fine-Tune-Best-Frequency, Central Controller calculates a "best fit" frequency for the reported graph of FREQUENCY-WEIGHT as a function of FREQUENCY. One example of such calculation would be to look for the best fit between a obtained graph and a theoretical gaussian graph; while such a calculation is "too heavy" for the MCU, it can be made on Central Controller easily.

Device after-Zero-Pairing
-------------------------

For Devices with Zero Pairing, the following procedure is used: 

* From Zero Pairing, Device gets pre-programmed list of frequencies for "reduced scan", based on SmartAnthill known-frequency; these frequencies SHOULD be expressed in terms which are convenient for the Device to be used; in particular, they SHOULD be recalculated into prefered-Device's form, and SHOULD be expressed as (start,end,increment). These frequencies MUST be calculated to cover range from `SA-frequency - 2e-4 * SA-frequency` to `SA-frequency + 2e-4 * SA-frequency`, with a step of `SA-deviation / 2`. Zero Pairing DOES NOT set field 'preferred-frequency' for the Device.
* When Device is turned on for the first time after being programmed with Zero Pairing, it has no preferred-frequency in EEPROM, so it:

  - takes one of the frequencies from the list of frequencies obtained from Zero Pairing
  - performs SAMP PHY quality measurement (as described in :ref:`samp` document), with the following clarifications:

    + `frequency-quality` variable is set to 0
    + measurement is performed over 5 packets sent
    + for each packet sent, there can be multiple packets received (as described in :ref:`samp`)
    + for each packet received, number-of-erroneous-bits (based on data from Hamming decoder) is calculated (if applicable). 
    + for each packet received, `weight = 2^24 >> number-of-erroneous-bits`, is added to frequency-quality
  
  - repeats the process for another frequency from the list
  - the frequency with the largest `frequency-quality` becomes preferred-frequency (up until the Frequency-Fine-Tuning described below).
  - from this point on, Device uses this preferred-frequency
  - Device sends a Fine-Tune-Best-Frequency packet to Central Controller, with all the data gathered from the measurements above
  - Device receives a Fine-Tune-Best-Frequency reply, double-checks it for sanity (TODO: what if insane?), writes received preferred-frequency to EEPROM, and starts to use preferred-frequency for all the subsequent communications
  - Device sends a PHY-Data-Ready-Response (sic!), and receives PHY-Data-Ready-Request (sic!). From this point on, Device is ready to work within the SmartAnthill PAN.

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

**\| ENCODING-TYPE \| SADLP-RF-DATA \|**

where ENCODING-TYPE is 1-byte fields (see below).

ENCODING-TYPE is an error-correctable field, described by the following table:

+------------------------+---------------------------------------+-------------------------------+
| ENCODING-TYPE          | Meaning                               | Value after Hamming Decoding  | 
+------------------------+---------------------------------------+-------------------------------+
| 0x00                   | RESERVED (NOT RECOMMENDED)            | 0                             |
+------------------------+---------------------------------------+-------------------------------+
| 0x69                   | RESERVED (MANCHESTER-COMPATIBLE)      | 1                             |
+------------------------+---------------------------------------+-------------------------------+
| 0xAA                   | RESERVED (MANCHESTER-COMPATIBLE)      | 2                             |
+------------------------+---------------------------------------+-------------------------------+
| 0xC3                   | PLAIN16-NO-CORRECTION                 | 3                             |
+------------------------+---------------------------------------+-------------------------------+
| 0xCC                   | HAMMING-32-CORRECTION                 | 4                             |
+------------------------+---------------------------------------+-------------------------------+
| 0xA5                   | RESERVED (MANCHESTER-COMPATIBLE)      | 5                             |
+------------------------+---------------------------------------+-------------------------------+
| 0x66                   | RESERVED (MANCHESTER-COMPATIBLE)      | 6                             |
+------------------------+---------------------------------------+-------------------------------+
| 0x0F                   | RESERVED                              | 7                             |
+------------------------+---------------------------------------+-------------------------------+
| 0xF0                   | RESERVED                              | 8                             |
+------------------------+---------------------------------------+-------------------------------+
| 0x99                   | RESERVED (MANCHESTER-COMPATIBLE)      | 9                             |
+------------------------+---------------------------------------+-------------------------------+
| 0x5A                   | RESERVED (MANCHESTER-COMPATIBLE)      | 10                            |
+------------------------+---------------------------------------+-------------------------------+
| 0x33                   | HAMMING-32-2D-CORRECTION              | 11                            |
+------------------------+---------------------------------------+-------------------------------+
| 0x3C                   | RESERVED                              | 12                            |
+------------------------+---------------------------------------+-------------------------------+
| 0x55                   | RESERVED (MANCHESTER-COMPATIBLE)      | 13                            |
+------------------------+---------------------------------------+-------------------------------+
| 0x96                   | RESERVED (MANCHESTER-COMPATIBLE)      | 14                            |
+------------------------+---------------------------------------+-------------------------------+
| 0xFF                   | RESERVED (NOT RECOMMENDED)            | 15                            |
+------------------------+---------------------------------------+-------------------------------+

All listed ENCODING-TYPEs have "Hamming Distance" of at least 4 between them. It means that error correction can be applied to ENCODING-TYPE, based on "Hamming Distance", as described below (for error correction to work, "Hamming Distance" must be at least 3).

ENCODING-TYPE can be considered as a Hamming (7.4) code as described in https://en.wikipedia.org/wiki/Hamming_code, with a prepended parity bit to make it SECDED. Note: implementation is not strictly required to perform Hamming decoding; instead, the following procedure MAY be used for error correction of ENCODING-TYPE:

* calculate "Hamming Distance" of received ENCODING-TYPE with one of supported values (NO-CORRECTION, HAMMING-32-CORRECTION, and HAMMING-32-2D-CORRECTION)
* if "Hamming Distance" is 0 or 1, than we've found the error-corrected ENCODING-TYPE
* otherwise - repeat the process with another supported value
* if we're out of supported values - ENCODING-TYPE is beyond repair, and we SHOULD drop the whole packet

To check that "Hamming Distance" of bytes a and b is <=1:

* calculate d = a XOR b
* calculate number of 1's in d

  + if MCU supports this as an asm operation - it is better to use it
  + otherwise, either shift-and-add-if
  + or compare with each of (0,1,2,4,8,16,32,64,128) - if doesn't match any, "Hamming Distance" is > 1

PLAIN16 Block
^^^^^^^^^^^^^

PLAIN16 block is always a 16-bit (2-byte) block. It consists of 15 data bits d0..d15, followed by 16th bit p, where p = ~d15 (inverted d15). p is necessary to provide strict guarantees that there is at least 1 bit change every 16 bits of data stream. On receiving side, p is ignored (though if bit-error counter is enabled, and p it is not equal to ~d15, it SHOULD be counted as a bit-error). 

Converting Data Block into a Sequence of PLAIN16 Blocks
'''''''''''''''''''''''''''''''''''''''''''''''''''''''

To produce PLAIN16-BLOCK-SEQUENCE from DATA-BLOCK, the following procedure is used:

* PADDED-DATA-BLOCK is formed as **\| DATA-BLOCK \| padding \|**, where padding is random data (using non-key random stream as specified in :ref:`sarng`) with a size, necessary to make the bitsize of PADDED-DATA-BLOCK a multiple of 15. *NB: Within implementation, PADDED-DATA-BLOCK is usually implemented virtually*
* resulting bit sequence (which has bitsize which is a multiple of 15) is split into 15-bit chunks, and each 15-bit chunk is converted into a 16-bit PLAIN16 block

PLAIN16-NO-CORRECTION Packets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For PLAIN16-NO-CORRECTION packets, SADLP-RF-DATA has the following format:

**\| SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD-PLAIN16 \|**

where SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD-PLAIN16 is a conversion of SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD into a sequence of PLAIN16 blocks, with SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD obtained by applying Salted-SCRAMBLED procedure (as described in :ref:`sascrambling` document) to payload from upper layer, and conversion is performed as described above.

In the absolutely worst case for PLAIN16-NO-CORRECTION packets, maximum distance between edges is always <= 15. 

HAMM32 block
^^^^^^^^^^^^

HAMM32 block is always a 32-bit (4-byte) block. It is a Hamming (31,26)-encoded block where d1..d26 are data bits and p1,p2,p4,p8,p16 are parity bits as described in https://en.wikipedia.org/wiki/Hamming_code, then HAMM32 block is built as follows:

**\| p0 \| ~p1 \| ~p2 \| d1 \| ~p4 \| d2 \| d3 \| d4 \| ~p8 \| d5 \| d6 \| d7 \| d8 \| d9 \| d10 \| d11 \| ~p16 \| d12 \| d13 \| d14 \| d15 \| d16 \| d17 \| d18 \| d19 \| d20 \| d21 \| d22 \| d23 \| d24 \| d25 \| d26 \|**

where '~' denotes bit inversion, and p0 is calculated to make the whole 32-bit HAMM32 parity even (making HAMM32 a SECDED block).

Parity bit inversion is needed to make sure that HAMM32 block can never be all-zeros or all-ones (and simple inversion doesn't change Hamming Distances, so error correction on the receiving side is essentially the same as for non-inverted parity bits). HAMM32 blocks guarantee that there is at least one change-from-zero-to-one-or-vice-versa at least every 32 bits. 

Converting Data Block into a Sequence of HAMM32 Blocks
''''''''''''''''''''''''''''''''''''''''''''''''''''''

To produce HAMM32-BLOCK-SEQUENCE from DATA-BLOCK, the following procedure is used:

* PADDED-DATA-BLOCK is formed as **\| DATA-BLOCK \| padding \|**, where padding is random data (using non-key random stream as specified in :ref:`sarng`) with a size, necessary to make the bitsize of PADDED-DATA-BLOCK a multiple of 26. *NB: Within implementation, PADDED-DATA-BLOCK is usually implemented virtually*
* resulting bit sequence (which has bitsize which is a multiple of 26) is split into 26-bit chunks, and each 26-bit chunk is converted into a 32-bit HAMM32 block

HAMMING-32-CORRECTION Packets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For HAMMING-32-CORRECTION packets, SADLP-RF-DATA is **\| SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD-HAMM32 \|**

where SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD-HAMM32 is a conversion of SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD into a sequence of HAMM32 blocks, with SALTED-SCRAMBLED-UPPER-LAYER-PAYLOAD obtained by applying Salted-SCRAMBLED procedure (as described in :ref:`sascrambling` document) to payload from upper layer, and conversion is performed as described above.

In the absolutely worst case for HAMMING-32-CORRECTION packets, maximum distance between edges is always <= 39. However, given Salted-SCRAMBLING, it is statistically MUCH better than that.

HAMMING-32-2D-CORRECTION Packets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

HAMMING-32-2D-CORRECTION is similar to HAMMING-32-CORRECTION, with an additional field of 2D-HAMM32 being added.

2D-HAMM32 consists of 26 additional Hamming checksums; each Hamming checksum #i consists of N parity bits of Hamming code, calculated over all bits #i in 26-bit data bits within HAMM32 blocks forming UPPER-LAYER-PAYLOAD-HAMM32. Number N is a number of Hamming bits necessary to provide error correction for NN=NUMBER-OF-HAMM32-BLOCKS. Hamming checksums are encoded as a bitstream, without intermediate padding, but padded at the end to a byte boundary with random (non-key-stream) data.

For example, if original block is 50 bytes long, then it will be split into 16 26-bit blocks, which will be encoded as 16 HAMM32 blocks (to foem UPPER-LAYER-PAYLOAD-HAMM32); then, for HAMMING-32-2D-CORRECTION, additional 26 Hamming checksums (5 bits each, as for NN=16 N=5) will be added. Therefore, original 50 bytes will be encoded as 4*16+17=81 byte (62% overhead).

