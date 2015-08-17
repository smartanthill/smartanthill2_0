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

.. _siot:

SimpleIoT Protocol Stack
========================

:Version: 0.1

SimpleIoT protocol stack is intended to provide communication services over heterogeneous IoT networks, allowing SimpleIoT Clients to control SimpleIoT Devices. These communication services are implemented as request-response services within OSI/ISO network model. SimpleIoT Devices and Clients connected together, form SimpleIoT Personal Area Network (PAN).

SimpleIoT protocol stack consists of several protocols, which adhere to a common naming schema, and currently include SimpleIoT/VM, SimpleIoT/CCP (which includes SimpleIoT/Pairing and SimpleIoT/OtAProgramming subprotocols), SimpleIoT/GDP, SimpleIoT/SP, SimpleIoT/oIP, SimpleIoT/MP, and SimpleIoT/DLP-\*. Details of these protocols are described below.

Design Goals
------------

SimpleIoT protocol stack is aimed to be used as a communication stack of an operating system such as Zepto OS, FreeRTOS or Contiki. As a result the following design goals apply:

* SimpleIoT is a heterogeneous network protocol, aimed to provide interoperability between Devices which have very different connectivity means
* SimpleIoT aims to support a wide range of communication protocols, including both wired and wireless

  + SimpleIoT aims to support various wireless protocols, including FSK-modulated RF (specifying details of such modulation to ensure sufficient interoperability), 802.15.4, and Bluetooth.
  + SimpleIoT aims to support various wired protocols, including RS-232, CAN bus, and USB CDC ACM.

* SimpleIoT aims to support a wide range of MCUs, from 8-bit MCUs such as those AVR8-based, to 32-bit MCUs such as ARM Cortex-M0 to ARM Cortex-M4.

  + SimpleIoT for Terminating Devices should be small enough to fit into 16K-24K Flash (i.e. Zepto OS running on MCUs with 32K Flash should be perfectly usable, accounting for HAL, device drivers, etc.), and into 512 bytes RAM (i.e. Zepto OS running on MCUs with 1K RAM should be perfectly usable).
  + SimpleIoT for Retransmitting Devices should be small enough to fit into 24-32K Flash (i.e. Zepto OS running on MCUs with 64K Flash should be perfectly usable, accounting for HAL, device drivers, etc.), and into 3K bytes RAM (i.e. Zepto OS running on MCUs with 4K RAM should be perfectly usable).

* SimpleIoT aims to provide explicit support for battery-powered Terminating Devices, optimizing power consumption as much as possible, and allowing a Terminating Device to operate for months and years from a single 20-mm button cell (assuming that power consumption of the attached sensor is negligible). 

  + In particular, one very important scenario is enabling a (Terminating) SimpleIoT Device to turn off its receiver for a time period which is specified by SimpleIoT Client; during this time, Device is completely unaccessible by the rest of SimpleIoT network.
  + In addition, other power-optimization measures need to be employed where applicable, including reduction in number of packets transmitted and reduction in waiting times in "listening" state, transmission power wherever possible, and using forward error correction to allow for transmission with less emitted power and less retransmits.

.. contents::

Actors
------

In SimpleIoT Protocol Stack, there are three distinct actors:

* **SimpleIoT Client**. Whoever needs to control SimpleIoT Device(s). Each SimpleIoT Client needs to keep a SimpleIoT DB of those SimpleIoT Devices it needs to control. SimpleIoT Clients are usually implemented by Internet-connected PCs, though this is not strictly required. 
* **SimpleIoT Router**. SimpleIoT Router allows to control SimpleIoT Devices connected to it. Provides compression on SimpleIoT/oIP protocols, and implements SimpleIoT/DLP-\* protocols (see below). Each SimpleIoT Router keeps a SimpleIoT Routing Table, which is necessary to operate SimpleIoT mesh PAN (see [SimpleIoT/MP] document for details on SimpleIoT Mesh Protocol).
* **SimpleIoT Device**. Physical device (containing sensor(s) and/or actuator(s)), which implements at least some parts of SimpleIoT protocol stack. Every SimpleIoT Device runs its own (usually minimal and optimized for SimpleIoT tasks) IP stack (but does not necessarily run TCP stack). As described in [SimpleIoT/MP] document, SimpleIoT Devices are divided into Retransmitting Devices and Terminating Devices.

  + Terminating SimpleIoT Devices represent Devices which perform specific tasks (such as sensors and/or actuators). Terminating Devices do not retransmit packets in a SimpleIoT mesh network. Terminating Devices MAY turn off their receiver(s), and are good candidates to be battery-powered Devices.
  + Retransmitting SimpleIoT Devices MAY perform the same functions as Terminating Devices (i.e. controlling sensors and/or actuators), plus they SHOULD be able to retransmit packets to other SimpleIoT Devices within SimpleIoT mesh network. Retransmitting Devices MUST NOT turn off their receiver(s), and usually should have easy access to quite significant power sources.

Assumptions
-----------

SimpleIoT operates under assumption that most of communication in SimpleIoT PAN will happen between Device and Client (i.e. not between two Devices). Communication between Devices is currently supported only via a SimpleIoT Client; while this MIGHT change in the future versions, inter-Device communication will still be considered as a rare occurrence.

Addressing
----------

In SimpleIoT, each SimpleIoT Device is assigned it's own IPv6 address (usually generated pseudo-randomly as specified in RFC4193). When transferring SimpleIoT/oIP packets over SimpleIoT/MP, IPv6 and UDP headers MUST be compressed (as described in [SimpleIoT/oIP] document; techniques described there are similar to those of 6LoWPAN, but are more specific to SimpleIoT tasks, and are more efficient for our purposes as a result). 

In addition, each Device within PAN is assigned a NODE-ID (it happens as a part of "pairing" procedure, see [SimpleIoT/Pairing] document for details), which is used as a shortcut to IPv6 address whenever possible.


Relation between SimpleIoT protocol stack and OSI/ISO network model
-------------------------------------------------------------------

.. note::
    For more information, please scroll the table below horizontally

+--------+--------------+------------------+-------------------------+----------------------+----------------------------+------------------------+
| Layer  | OSI-Model    | SimpleIoT        |     Function            | Implementation       | Implementation             | Implementation         |
|        |              | Protocol Stack   |                         | on Clients           | on Routers                 | on Devices             |
|        |              |                  |                         |                      +---------------+------------+                        |
|        |              |                  |                         |                      | IP side       | SIoT side  |                        |
+========+==============+==================+=========================+======================+===============+============+========================+
| 7      | Application  | SimpleIoT/VM     | Device Control          | Byte-code Compiler   | --                         | SimpleIoT/VM           |
|        |              +------------------+-------------------------+----------------------+----------------------------+------------------------+
|        |              | SimpleIoT/CCP    | Command/Reply           | SimpleIoT/CCP        | --                         | SimpleIoT/CCP          |
|        |              |                  | Handling                |                      |                            |                        |
+--------+--------------+------------------+-------------------------+----------------------+----------------------------+------------------------+
| 5      | Session      | SimpleIoT/GDP    | Guaranteed              | SimpleIoT/GDP        | --                         | SimpleIoT/GDP          |
|        |              |                  | Delivery                | ("Master")           |                            | ("Slave")              |
|        |              +------------------+-------------------------+----------------------+----------------------------+------------------------+
|        |              | SimpleIoT/SP     | Encryption and          | SimpleIoT/SP         | SimpleIoT/SP (optional)    | SimpleIoT/SP           |
|        |              |                  | Authentication          |                      |                            |                        |
+--------+--------------+------------------+-------------------------+----------------------+---------------+------------+------------------------+
| 4      | Transport    | SimpleIoT/oIP    | Transport over IP       | SimpleIoT/oIP        | SimpleIoT/oIP |SimpleIoT/  | SimpleIoT/UDP          |
|        |              |                  | Networks                |                      |               |oUDP        | (compressed)           |
|        |              +------------------+-------------------------+----------------------+---------------+(compressed)|                        |
|        |              | UDP              | As usual for UDP        | UDP                  | UDP           |            |                        |
|        |              |                  |                         |                      |               |            |                        |
+--------+--------------+------------------+-------------------------+----------------------+---------------+------------+------------------------+
| 3      | Network      | SimpleIoT/MP     | Mesh for SimpleIoT/MP,  | IP                   | IP            | SimpleIoT/ | SimpleIoT/MP           |
|        |              | or IP            | As usual for IP         |                      |               | MP         |                        |
+--------+--------------+------------------+-------------------------+----------------------+---------------+------------+------------------------+
| 2      | Datalink     | SimpleIoT/DLP-\* | Intra-bus addressing,   | -- (standard network | -- (std netwk | SimpleIoT/ | SimpleIoT/DLP-\*       |
|        |              |                  | Fragmentation           | capabilities)        | capabilities) | DLP-\*     |                        |
|        |              |                  | (if applicable),        |                      |               |            |                        |
|        |              |                  | Forward Error Correction|                      |               |            |                        |
+--------+--------------+------------------+-------------------------+----------------------+---------------+------------+------------------------+
| 1      | Physical     | Physical         |                         | -- (standard network | -- (std netwk | Physical   | Physical               |
|        |              |                  |                         | capabilities)        | capabilities) |            |                        |
+--------+--------------+------------------+-------------------------+----------------------+---------------+------------+------------------------+

SimpleIoT protocol stack consists of the following protocols:

* **SimpleIoT/VM**. Essentially a byte-code interpreter, where byte-code is optimized for exteremely resource-constrained devices. SimpleIoT/VM handles generic commands and routes device-specific commands to device-specific plug-ins. Belongs to Layer 7 of OSI/ISO network model.

* **SimpleIoT/CCP** – SimpleIoT Command&Control Protocol. Also belongs to Layer 7 of OSI/ISO network model. 

* **SimpleIoT/GDP** – SimpleIoT Guaranteed Delivery Protocol. Belongs to Layer 5 of OSI/ISO network model. Provides guaranteed command/reply delivery. Flow control is implemented, but is quite rudimentary (only one outstanding packet is normally allowed for each virtual link, see details below). On the other hand, SimpleIoT/GDP provides efficient support for scenarios such as temporary disabling receiver on the SimpleIoT Device side; as noted above, such scenarios are very important to ensure energy efficiency.

* **SimpleIoT/SP** – SimpleIoT Security Protocol. Due to several considerations (including resource constraints) SimpleIoT protocol stack implements security on a layer right below SimpleIoT/GDP, so SimpleIoT/SP essentially belongs to Layer 5 of OSI/ISO network model.

* **SimpleIoT/oIP** – "SimpleIoT over IP" Protocol. MAY have different flavours, though currently only SimpleIoT/oUDP is supported. In the future support for SimpleIoT/oTCP MIGHT be added, but it won't be mandatory for Devices.

* **SimpleIoT/MP** - SimpleIoT Mesh Protocol. Aims to provide heterogeneous mesh network with an explicit "storm" control within applicable collision domains.

* **SimpleIoT/DLP-\*** – SimpleIoT DataLink Protocol family. Belongs to Layer 2 of OSI/ISO network model. SimpleIoT/DLP-\* is specific to an underlying transfer technology (so for CAN bus SimpleIoT/DLP-CAN is used, for IEEE 802.15.4 SimpleIoT/DLP-IEEE802.15.4 is used). Protocols from SimpleIoT/DLP-\* family handle fragmentation and forward error correction if necessary, and in general provide non-guaranteed packet transfer.


Error Handling Philosophy and Asymmetric Nature
-----------------------------------------------
In real-world operation, it is inevitable that from time to time a mismatch occurs between the states of SimpleIoT Client and SimpleIoT Device; while such mismatches should never occur as long as the SimpleIoT protocols are strictly adhered to, mistmatches still may occur for many practical reasons, such as reboot or restore-from-backup of SimpleIoT Client, a transient failure of the SimpleIoT Device (for example, due to power surge, near-depleted battery, RAM soft error due to X-rays, etc.).

SimpleIoT protocol stack attempts to clear as many such scenarios as possible 'automagically', without the need to reprogram SimpleIoT Device. To achieve this goal, the following approach is used: SimpleIoT protocol stack assumes that in any case when there is any kind of the mismatch, it is the SimpleIoT Client who's "right". In addition, if such a decision is not sufficient to recover from the mismatch, SimpleIoT Device will perform complete re-initialization.

It means that certain SimpleIoT protocols (such as SimpleIoT/CCP and SimpleIoT/GDP) are inherently asymmetrical; details are provided in their respective documents ([SimpleIoT/CCP] and [SimpleIoT/GDP] respectively).

TODO: recommend on-device watchdog?

Packet Chains
-------------

SimpleIoT protocol stack is intended to provide various services between two entities: SimpleIoT Client and SimpleIoT Device. Most of these services are of request-response nature. To implement them while imposing the least requirements on the resource-stricken SimpleIoT Device, all interactions within SimpleIoT protocol stack at the levels between SimpleIoT/VM and SimpleIoT/GDP (inclusive) are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on.

Packet chains are initiated at SimpleIoT/VM layer, and are supported by all the layers between SimpleIoT/VM and SimpleIoT/GDP (inclusive). Whenever SimpleIoT/VM issues a packet to an underlying protocol, it MUST specify whether a packet is a first, intermediate, or last within a “packet chain”. This information allows underlying protocols (down to SimpleIoT/GDP) to arrange for proper retransmission if some packets are lost during communication, see [SimpleIoT/GDP] document for details.

Packet Size Guarantees, DEVICECAPS instruction, SIMPLEIOT_VM_GUARANTEED_PAYLOAD, and Fragmentation
--------------------------------------------------------------------------------------------------

All SimpleIoT Devices MUST support sending SimpleIoT/VM commands and receiving SimpleIoT/VM replies with at-least-8-bytes payload; all underlying protocols MUST support it (taking into account appropriate header sizes, so, for example, SimpleIoT/SP MUST be able to pass at least 8_bytes+SimpleIoT_VM_headers+SimpleIoT_CCP_headers+SimpleIoT_GDP_headers as payload). If Client needs to send a command which is larger than 8 bytes, it SHOULD obtain information about device capabilities, before doing it. It SHOULD be done via SimpleIoT/VM DEVICECAPS request (see [SimpleIoT/VM] for details). When Client doesn't have information about Device, it's SimpleIoT/VM request with the DEVICECAPS instruction MUST be <= 8 bytes in size; VM's reply to a DEVICECAPS instruction MAY be larger than 8 bytes if it is specified in the instruction (and if is Device itself is capable of sending it). The information obtained from DEVICECAPS request, SHOULD be stored in Client's SimpleIoT DB.

One of DeviceCapabilities fields is SIMPLEIOT_VM_GUARANTEED_PAYLOAD (which is conceptually similar to MTU from IP stack, but includes header sizes to provide information which is appropriate for Layer 7). When a SimpleIoT Device fills in SIMPLEIOT_VM_GUARANTEED_PAYLOAD in response to DEVICECAPS request, it MUST take into account capabilities of it's L1/L2 protocol; that is, if a SimpleIoT Device supports IEEE 802.15.4 and L2 protocol which doesn't perform packet fragmentation and re-assembly, then the Device won't be able to send/receive payloads which are roughly 80 bytes in size (exact size depends on headers and needs to be calculated depending on protocol specifics), and it MUST NOT report DeviceCapabilities.SIMPLEIOT_VM_GUARANTEED_PAYLOAD which is more than this amount. TODO: separate _COMMAND/_REPLY instead of _PAYLOAD?

In SimpleIoT, fragmentation and re-assembly is a responsibility of SimpleIoT/DLP-\* family of protocols. If implemented, it may allow device to increase reported (and sent/received) SIMPLEIOT_VM_GUARANTEED_PAYLOAD. 

All SimpleIoT Retransmitting Devices MUST support SIMPLEIOT_VM payload sizes of at least 384 bytes. Therefore, after obtaining Device Capabilities for a SimpleIoT Device, SimpleIoT Client MAY rely on *min(DeviceCapabilities.SIMPLEIOT_VM_GUARANTEED_PAYLOAD,384)* being guaranteed to be delivered to the Device. 

Stack-Wide Encodings
--------------------

There are some encodings and encoding conventions which are used throughout SimpleIoT Protocol Stack. 

SimpleIoT Encoded-Unsigned-Int
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In several places in SimpleIoT Protocol Stack, there is a need to encode integers, which happen to be small most of the time (one such example is sizes, another example is some kinds of incrementally-increased ids such as NODE-ID). To encode them efficiently, SimpleIoT Protocol Stack uses a compact encoding, which encodes small integers with smaller number of bytes. Encoded-Unsigned-Int is very close to *Variable-length quantity (VLQ)* (see http://en.wikipedia.org/wiki/Variable-length_quantity), however, SimpleIoT Encoded-Unsigned-Int<> encoding enforces "canonical" VLQ representation, prohibiting non-optimal encodings such as two-byte encoding of '0'. Also note that other encodings such as Encoded-Signed-Int are different from what is described on VLQ Wikipedia page.

Encoded-Unsigned-Int is a variable-length encoding of unsigned integers. Namely:

* if the first byte of Encoded-Unsigned-Int is c1 <= 127, then the value of Encoded-Unsigned-Int is equal to c1
* if the first byte of Encoded-Unsigned-Int is c1 >= 128, then the next byte c2 is needed:

  + if the second byte of Encoded-Unsigned-Int is c2 <= 127, then the value of Encoded-Unsigned-Int is equal to *((uint16)(c1&0x7F) | ((uint16)c2 << 7))*.
  + if the second byte of Encoded-Unsigned-Int is c2 >= 128, then the next byte c3 is needed:
    
    * if the third byte of Encoded-Unsigned-Int is c3 <= 127, then the value of Encoded-Unsigned-Int is equal to *((uint32)(c1&0x7F) | ((uint32)(c2&0x7F) << 7)) | ((uint32)c3 << 14))*.
    * if the third byte of Encoded-Unsigned-Int is c3 >= 128, then the next byte c4 is needed:

      + if the fourth byte of Encoded-Unsigned-Int is c4 <= 127, then the value of Encoded-Unsigned-Int is equal to *((uint32)(c1&0x7F) | ((uint32)(c2&0x7F) << 7)) | ((uint32)(c3&0x7F) << 14)) | ((uint32)c4 << 21))*.
      + if the fourth byte of Encoded-Unsigned-Int is c4 >= 128, then the next byte c5 is needed.

        * for nth byte:

          + if the nth byte of Encoded-Unsigned-Int is cn <= 127, then the value of Encoded-Unsigned-Int is equal to *((uintNN)(c1&0x7F) | ((uintNN)(c2&0x7F) << 7)) | ((uintNN)(c3&0x7F) << 14)) | ... | ((uintNN)(c<n-1>&0x7F) << (7*(n-2))))) | ((uintNN)cn << (7*(n-1))))*, where uintNN is sufficient to store the result. *NB: in practice, for Encoded-Unsigned-Ints over 4 bytes, implementation is likely to be quite different from, but equivalent to, the formula given*
          + if the nth byte of Encoded-Unsigned-Int is cn >= 128, then the <n+1>th byte is needed.

IMPORTANT: Encoded-Unsigned-Int enforces "canonical" representation. It means that all integers MUST be encoded with the smallest number of bytes possible. This requirement is equivalent to a requirement that for encodings with length > 1, last byte of encoding MUST NOT be equal to zero. This MUST be checked by compliant implementations (and MUST generate invalid-encoding exception, with effects depending on the point where it has occurred). 
 
The following table shows how many Encoded-Unsigned-Int bytes is necessary to encode ranges of Encoded-Unsigned-Int values:

+-------------------------+---------------------+------------------+------------------+
| Encoded-Unsigned-Int    | Encoded-Unsigned-Int| Fully Covers     | Result fits in   |
| Values                  | Bytes               |                  |                  |
+=========================+=====================+==================+==================+
| 0-127                   | 1                   | 7 bits           | 1 byte           |
+-------------------------+---------------------+------------------+------------------+
| 128-16 383              | 2                   | 14 bits          | 2 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 16 512-2 097 151        | 3                   | 21 bits          | 3 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 2 097 152-268 435 455   | 4                   | 28 bits          | 4 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 268 435 456-            | 5                   | 35 bits          | 5 bytes          |
| 34 359 738 367          |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 34 359 738 368-         | 6                   | 42 bits          | 6 bytes          |
| 4 398 046 511 103       |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 4 398 046 511 104-      | 7                   | 49 bits          | 7 bytes          |
| 562 949 953 421 311     |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 562 949 953 421 312-    | 8                   | 56 bits          | 8 bytes          |
| 72 057 594 037 927 935  |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
|72 057 594 037 927 936-  | 9                   | 63 bits          | 8 bytes          |
|9 223 372 036 854 775 808|                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+

IMPORTANT: Encoding-Unsigned-Int encoding (specifically, low-to-high byte encoding order) guarantees that for even numbers, first byte of encoded value is always even. This property MAY be relied on in other places in protocol stack, specifically, in "indicate an error in an unknown-length field" scenarios (so if we decide to change order of bytes in the encoding, we need to change logic in those places too). 

Table of correspondence of "max=" parameter and maximum possible encoding length: 

+---------------------+---------------------------------------+
| max=                | maximum Encoded-Unsigned-Int bytes    |
+=====================+=======================================+
| 1                   | 2                                     |
+---------------------+---------------------------------------+
| 2                   | 3                                     |
+---------------------+---------------------------------------+
| 3                   | 4                                     |
+---------------------+---------------------------------------+
| 4                   | 5                                     |
+---------------------+---------------------------------------+
| 5                   | 6                                     |
+---------------------+---------------------------------------+
| 6                   | 7                                     |
+---------------------+---------------------------------------+
| 7                   | 8                                     |
+---------------------+---------------------------------------+
| 8                   | 10                                    |
+---------------------+---------------------------------------+

Encoded-Signed-Int
''''''''''''''''''

Encoded-Signed-Int is an encoding for signed integers, based on Zig-Zag conversion from signed integer to unsigned integer, and subsequent Encoded-Unsigned-Int encoding of unsigned integer. 

Zig-Zag conversion is the same as described here: https://developers.google.com/protocol-buffers/docs/encoding?csw=1#types. For example, to convert int16_t *sx* to uint16_t *ux*, the following C language expression is used: 

`ux = (uint16_t)((sx << 1) ^ (sx>>15))`

To convert int32_t *sx* to uint32_t *ux*, expression becomes `ux = (uint32_t)((sx << 1) ^ (sx>>31))`, and so on. 

Note that right shift in these expressions is a signed shift, making it equivalent creating a bitmask of appropriate length, consisting out of all '0' or out of all '1's (equal to the sign bit of original signed integer). This allows, for example, to calculate one byte of this mask by signed-shifting highest byte of *sx* to the right by 7, and then to use this byte for XORing with all the bytes of left-shifted sx; this trick should speed up implementations on 8-bit MCUs. 

After *ux* is calculated, it is stored as an Encoded-Unsigned-Int of the appropriate size, as described above.

To perform Zig-Zag conversion back (from Zig-Zag-encoded unsigned *ux* to original signed *sx*), the following expression may be used (for 16-bit conversions, for the others expressions are very similar):

`sx = (int16_t)((ux >> 1) ^ (-(ux & 1)))`

Note that once again, all bits (and therefore bytes) of `(-(ux&1))` are the same, so one byte can be calculated (this time - based on lowest byte) and then used for XORing with all the bytes of right-shifted *ux*.

Encoded-\*-Int<max=>
''''''''''''''''''''

Wherever SimpleIoT specification mentions Encoded-Unsigned-Int or Encoded-Signed-Int, it MUST specify it in the form of *Encoded-Unsigned-Int<max=...>* or *Encoded-Signed-Int<max=...>*. "max=" parameter specifies maximum number of bytes which are necessary to represent the encoded number. For example, Encoded-Unsigned-Int<max=2> specifies that the number is between 0 and 65535 (and therefore from one to three bytes may be used to encode it). The high bit of the last possible byte of Encoded-\*-Int is always 0; this ensures an option for an easy expansion in the future.

Currently supported values of "max=" parameter are from 1 to 8.

When parsing Encoded-\*-Int, if high bit in the last-possible byte is 1, then Encoded-\*-Int is considered invalid. Handling of invalid Encoded-\*-Ints SHOULD be specified in the appropriate place of documentation.

SimpleIoT Endianness
^^^^^^^^^^^^^^^^^^^^

In most cases, SimpleIoT Protocol Stack uses SimpleIoT Encoded-\*-Int<max=...> to encode integers. However, there are some cases where we need an exact number of bytes, and have no idea about their statistical distribution. In such cases, using Encoded-\*-Int<> would be a waste. 

In such cases, SimpleIoT uses **SimpleIoT Endianness**, which is **LITTLE-ENDIAN**.

*Rationale for using LITTLE-ENDIAN encoding (rather than "network byte order" which is traditionally big-endian) is based on the observation that the most resource-constrained MCUs out of target group (such as PIC and AVR8), are little-endian. For them, the difference of not doing conversion between protocol-order and MCU-order might be important; as the other MCUs are not that much constrained, we don't expect the cost of conversion to be significant. In other words, this LITTLE-ENDIAN decision to favours poorer-resource MCUs at the cost of richer-resource MCUs.*

SimpleIoT Bitfields
^^^^^^^^^^^^^^^^^^^

In some cases, SimpleIoT Protocols use bitfields; in such cases: 

* bitfields MUST use 1-byte, 2-byte, Encoded-Unsigned-Int<max=>, or Encoded-Signed-Int<max=> field as a 'substrate'. 'Bitfield Substrate' is composed/parsed as an ordinary field, which is encoded using appropriate encodings described in this document.
* as soon as 'substrate' is parsed, it is treated as an integer, out of which specific bits can be used; these bits are specified as [3] (specifying that single bit #3 is used), or [2..4] (specifying that bits from 2 to 4 - inclusive - are used). Bit[0] means the least significant bit, i.e. (substrate&0x01), bit[1] - the next bit, i.e. ((substrate>>1)&0x01), and so on.
* if 'substrate' is an Encoded-Unsigned-Int field, then one of bitfields MAY be specified as [2..] - specifying that all the bits from 2 to the highest available one, are used for the bitfield.
* if 'substrate' is an Encoded-Signed-Int field, then one of bitfields MAY be specified as [2..] - specifying that all the bits from 2 to the highest available one, are used for the bitfield; in this example, the bitfield in question MUST be calculated as `substrate>>1`, where substrate is treated as signed (i.e. '>>' operator works extending sign bit).

SimpleIoT Half-Float
^^^^^^^^^^^^^^^^^^^^

Some SimpleIoT packets (in particular, some of the commands within SimpleIoT/VM) use 'Half-Float' data as described here: http://en.wikipedia.org/wiki/Half-precision_floating-point_format. SimpleIoT serializes such data as 2-byte substrate (encoded according to SimpleIoT Endianness), then considering Sign-Bit bitfield as bit [15], Exponent bitfield as bits [10..14], and Fraction bitfield as bits [0..9]. This representation is strictly equivalent to the one described in Wikipedia (TODO: check).

