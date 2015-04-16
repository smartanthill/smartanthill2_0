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

.. _saprotostack:

SmartAnthill 2.0 Protocol Stack
===============================

:Version:   v0.2.8c

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill protocol stack is intended to provide communication services between SmartAnthill Clients and SmartAnthill Devices, allowing SmartAnthill Clients to control SmartAnthill Devices. These communication services are implemented as a request-response services within OSI/ISO network model.

.. contents::


Actors
------

In SmartAnthill Protocol Stack, there are three distinct actors:

* **SmartAnthill Client**. Whoever needs to control SmartAnthill Device(s). SmartAnthill Clients are usually implemented by SmartAnthill Central Controllers, though this is not strictly required. 
* **SmartAnthill Router**. SmartAnthill Router allows to control SmartAnthill Devices connected to it. Performs conversion between SAoIP and SADLP-\* protocols (see below).
* **SmartAnthill Device**. Physical device (containing sensor(s) and/or actuator(s)), which implements at least some parts of SmartAnthill protocol stack. SmartAnthill Devices can be further separated into:

  + **SmartAnthill Simple Device**. SmartAnthill Device which is not powerful enough to run it's own SAoIP/IP stack. MUST be connected via SmartAnthill Router.
  + **SmartAnthill IP-enabled Device**. SmartAnthill Device which is enabled to run it's own SAoIP/IP stack. MAY be connected without SmartAnthill Router directly to Intranet/Internet. Note that implementing TCP is not strictly required for SmartAnthill IP-enabled Devices.

Addressing
----------

In SmartAnthill, each SmartAnthill Device (whether Simple Device or IP-enabled device) is assigned it's own unique *SmartAnthill Address*, which is a triplet (IPv6-address:SAoIP-subprotocol:port-number). It allows to have either one IP address per device, or to have multiple devices per IP address as necessary and/or convenient. Currently supported SAoIP-subprotocols (also known as "SAoIP flavours") are: UDP, TCP, TLSoTCP (with port-number being UDP port number for UDP subprotocol, and TCP port number for TCP and TLSoTCP subprotocols). For further details, please refer to :ref:`saoip` document. For *SmartAnthill Simple Devices*, this SA 'triplet' address is translated into L2 bus-specific address by SmartAnthill Router.


Relation between SmartAnthill protocol stack and OSI/ISO network model
----------------------------------------------------------------------

.. note::
    For more detailed information please scroll table below by horizontal

+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| Layer  | OSI-Model    | SmartAnthill     |     Function          | Implementation       | Implementation         | Implementation             | Implementation         |
|        |              | Protocol Stack   |                       | on Clients           | on IP-enabled Devices  | on Routers                 | on Simple Devices      |
|        |              |                  |                       |                      |                        +---------------+------------+                        |
|        |              |                  |                       |                      |                        | IP side       | SA side    |                        |
+========+==============+==================+=======================+======================+========================+===============+============+========================+
| 7      | Application  | Zepto VM         | Device Control        | Control Program      | Zepto VM               | --                         | Zepto VM               |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| 6      | Presentation | SACCP            | Command/Reply         | SACCP                | SACCP                  | --                         | SACCP                  |
|        |              |                  | Handling              |                      |                        |                            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| 5      | Session      | SAGDP            | Guaranteed            | SAGDP ("Master")     | SAGDP ("Slave")        | --                         | SAGDP ("Slave")        |
|        |              |                  | Delivery              |                      |                        |                            |                        |
|        |              +------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
|        |              | SASP             | Encryption and        | SASP                 | SASP                   | SASP (optional)            | SASP                   |
|        |              |                  | Authentication        |                      |                        |                            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+
| 4      | Transport    | SAoIP [1]_       | Transport over IP     | SAoIP                | SAoIP                  | SAoIP         | --         | --                     |
|        |              |                  | Networks              |                      |                        |               |            |                        |
|        |              +------------------+-----------------------+----------------------+------------------------+---------------+            |                        |
|        |              | TCP or UDP       | As usual for TCP/UDP  | TCP or UDP           | TCP or UDP             | TCP or UDP    |            |                        |
|        |              |                  |                       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+            |                        |
| 3      | Network      | IP               | As usual for IP       | IP                   | IP                     | IP            |            |                        |
|        |              |                  |                       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+
| 2      | Datalink     | SADLP-\*         | Intra-bus addressing, | -- (standard network | -- (standard network   | -- (std netwk | SADLP-*    | SADLP-*                |
|        |              |                  | Fragmentation         | capabilities)        | capabilities)          | capabilities) |            |                        |
|        |              |                  | (if applicable)       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+
| 1      | Physical     | Physical         |                       | -- (standard network | -- (standard network   | -- (std netwk | Physical   | Physical               |
|        |              |                  |                       | capabilities)        | capabilities)          | capabilities) |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+

.. [1] For Simple SmartAnthill Devices, SAoIP is translated into SADLP-\* by the SmartAnthill Router (the one which directly controls the SmartAnthill Device). It can (and SHOULD) be done in a completely transparent manner, so that SmartAnthill Client SHOULD be able to communicate with SmartAnthill Device in exactly the same manner regardless of SmartAnthill Device being IP-enabled Device or Simple Device.

SmartAnthill protocol stack consists of the following protocols:

* **Zepto VM**. Essentially a byte-code interpreter, where byte-code is optimized for exteremely resource-constrained devices. Zepto VM handles generic commands and routes device-specific commands to device-specific plug-ins.

* **SACCP** – SmartAnthill Command&Control Protocol. Corresponds to Layer 7 of OSI/ISO network model. 

* **SAGDP** – SmartAnthill Guaranteed Delivery Protocol. Belongs to Layer 5 of OSI/ISO network model. Provides guaranteed command/reply delivery. Flow control is implemented, but is quite rudimentary (only one outstanding packet is normally allowed for each virtual link, see details below). On the other hand, SAGDP provides efficient support for scenarios such as temporary disabling receiver on the SmartAnthill Device side; such scenarios are very important to ensure energy efficiency.

* **SASP** – SmartAnthill Security Protocol. Due to several considerations (including resource constraints) SmartAnthill protocol stack implements security on a layer right below SAGDP, so SASP essentially belongs to Layer 5 of OSI/ISO network model.

* **SAoIP** – SmartAnthill over IP Protocol. Lies right on top of TLS, TCP or UDP. SAoIP is not implemented on SmartAnthill Simple Devices, and all the SAoIP headers are stripped (and replaced with L2 headers) by SmartAnthill Router before passing the data to SmartAnthill Simple Device.

* **SADLP-\*** – SmartAnthill DataLink Protocol family. Belongs to Layer 2 of OSI/ISO network model. SADLP-\* is specific to an underlying transfer technology (so for CAN bus SADLP-CAN is used, for IEEE 802.15.4 SADLP-IEEE802.15.4 is used). SADLP-\* handles fragmentation if necessary and provides non-guaranteed packet transfer.


Error Handling Philosophy and Asymmetric Nature
-----------------------------------------------
In real-world operation, it is inevitable that from time to time a mismatch occurs between the states of SmartAnthill Central Controller and SmartAnthill Device; while such mismatches should never occur as long as the SmartAnthill protocols are strictly adhered to, mistmatches still may occur for many practical reasons, such as reboot or restore-from-backup of SmartAnthill Central Controller, a transient failure of the SmartAnthill Device (for example, due to power surge, near-depleted battery, RAM soft error due to cosmic rays, etc.).

SmartAnthill protocol stack attempts to clear as many such scenarios as possible 'automagically', without the need to reprogram SmartAnthill Device. To achieve this goal, the following approach is used: SmartAnthill protocol stack assumes that in any case when there is any kind of the mismatch, it is the SmartAnthill Central Controller who's "right". In addition, if such a decision is not sufficient to recover from the mismatch, SmartAnthill Device will perform complete re-initialization.

It means that certain SmartAnthill protocols (such as SACCP and SAGDP) are inherently asymmetrical; details are provided in their respective documents ( :ref:`saccp`  and :ref:`sagdp` ).

TODO: recommend on-device self-recovery circuit?


Packet Chains
-------------

SmartAnthill protocol stack is intended to provide various services between two entities: SmartAnthill Central Controller and SmartAnthill Device. Most of these services are of request-response nature. To implement them while imposing the least requirements on the resource-stricken SmartAnthill Device, all interactions within SmartAnthill protocol stack at the levels between SACCP and SAGDP (inclusive) are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on.

Chains are initiated by the topmost protocol is SmartAnthill protocol layer, SACCP, and are supported by all the layers between SACCP and SAGDP (inclusive). Whenever SACCP issues a packet to an underlying protocol, it MUST specify whether a packet is a first, intermediate, or last within a “packet chain” (using 'is-first' and 'is-last' flags; note that due to “rules of engagement” described below, 'is-first' and 'is-last' flags are inherently incompatible, which MAY be relied on by implementation). This information allows underlying protocols (down to SAGDP) to arrange for proper retransmission if some packets are lost during communication, see :ref:`sagdp` document for details.

Starting from OSI Layer 2 and above, there is a virtual link established between SmartAnthill Central Controller and SmartAnthill Device. Normally (as guaranteed by SAGDP) only one outstanding packet is allowed on each such virtual link. There is one exception to this rule, which is described below.

Handling of temporary dual “packet chains”
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Normally, at each moment for each of the 'virtual links' decribed above, there can be only one “packet chain” active, and within a “packet chain”, all transmissions are always sequential. However, there are scenarios when both SmartAnthill Central Controller and SmartAnthill Device try to initiate their own “packet chains”. One such example is when SmartAnthill Device is sleeping according to instructions received from SmartAnthill Central Controller (and just woke up to perform task and report), and meanwhile SmartAnthill Central Controller has made a decision (for example, due to the input from other SmartAnthill Devices or from the end-user) to issue different set of instructions to the SmartAnthill Device.

Handling of these scenarios is explained in detail in respective documents ( :ref:`saccp` and :ref:`sagdp` ); as a result of such handling, one of the chains (the one coming from the SmartAnthill Device, according to "Central Controller is always right" principle described above), will be dropped pretty much as if it has never been started.

Packet Size Guarantees, DEVICECAPS instruction, SACCP_GUARANTEED_PAYLOAD, and Fragmentation
-------------------------------------------------------------------------------------------

In SmartAnthill, SACCP MUST allow sending commands with at-least-8-bytes payload; all underlying protocols MUST support it (taking into account appropriate header sizes, so, for example, SASP MUST be able to pass at least 8_bytes+SACCP_headers+SAGDP_headers as payload). If Client needs to send a command which is larger than 8 bytes, it SHOULD obtain information about device capabilities, before doing it. Currently, SmartAnthill provides two ways to do it:

* to obtain Device Capabilities information about SmartAnthill Device from SmartAnthill DB (see :ref:`saoverarch` document for details) at the time of SmartAnthill Device programming or "pairing". This method is currently beyond the scope of SmartAnthill Protocols (TODO: should we add it?).
* to obtain Device Capabilities information via Zepto VM DEVICECAPS instruction (see :ref:`sazeptovm` document for details). When Client doesn't have information about Device, it's SACCP request with Zepto VM's DEVICECAPS instruction MUST be <= 8 bytes in size; Zepto VM's SACCP  reply to a DEVICECAPS instruction MAY be larger than 8 bytes if it is specified in the instruction (and if is Device itself is capable of sending it).

One of DeviceCapabilities fields is SACCP_GUARANTEED_PAYLOAD (which is conceptually similar to MTU from IP stack, but includes header sizes to provide information which is appropriate for Layer 7). When SmartAnthill Device fills in SACCP_GUARANTEED_PAYLOAD in response to Device Capabilities request, it MUST take into account capabilities of it's L1/L2 protocol; that is, if a SmartAnthill Device supports IEEE 802.15.4 and L2 protocol which doesn't perform packet fragmentation and re-assembly, then the Device won't be able to send/receive payloads which are roughly 80 bytes in size (exact size depends on headers and needs to be calculated depending on protocol specifics), and it MUST NOT report DeviceCapabilities.SACCP_GUARANTEED_PAYLOAD which is more than this amount.

In SmartAnthill, fragmentation and re-assembly is a responsibility of SADLP-\* family of protocols. If implemented, it may allow device to increase reported (and sent/received) SACCP_GUARANTEED_PAYLOAD. 

All SmartAnthill Protocols, except for SADLP-\*, MUST support SACCP payload sizes of at least 384 bytes. Therefore, after obtaining Device Capabilities for a SmartAnthill Device, SmartAnthill Client MAY calculate *min(DeviceCapabilities.SACCP_GUARANTEED_PAYLOAD,384)* to determine SACCP payload size which is guaranteed to be delivered to the Device. Alternatively, SmartAnthill MAY calculate *min(DeviceCapabilities.SACCP_GUARANTEED_PAYLOAD,Client_Side_SACCP_Payload)* for the same purpose (here Client_Side_SACCP_Payload will depend on SAoIP protocol in use).

Stack-Wide Encodings
--------------------

There are some encodings and encoding conventions which are used throughout SmartAnthill Protocol Stack. 

SmartAnthill Encoded-Unsigned-Int
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In several places in SmartAnthill Protocol Stack, there is a need to encode integers, which happen to be small most of the time (one such example is sizes, another example is some kinds of incrementally-increased ids). To encode them efficiently, SmartAnthill Protocol Stack uses a compact encoding, which encodes small integers with smaller number of bytes. Encoded-Unsigned-Int is equivalent to *Variable-length quantity (VLQ)* as described here: http://en.wikipedia.org/wiki/Variable-length_quantity ("General structure" section); note that other encodings such as Encoded-Signed-Int are different from what is described on that page.

Encoded-Unsigned-Int is a variable-length encoding of integers (with the idea being somewhat similar to the idea behind UTF-8). Namely:

* if the first byte of Encoded-Unsigned-Int is c1 <= 127, then the value of Encoded-Unsigned-Int is equal to c1
* if the first byte of Encoded-Unsigned-Int is c1 >= 128, then the next byte c2 is needed:

  + if the second byte of Encoded-Unsigned-Int is c2 <= 127, then the value of Encoded-Unsigned-Int is equal to *128+((uint16)(c1&0x7F) | ((uint16)c2 << 7))*.
  + if the second byte of Encoded-Unsigned-Int is c2 >= 128, then the next byte c3 is needed:
    
    * if the third byte of Encoded-Unsigned-Int is c3 <= 127, then the value of Encoded-Unsigned-Int is equal to *16512+((uint32)(c1&0x7F) | ((uint32)(c2&0x7F) << 7)) | ((uint32)c3 << 14))* (note that 16512 is 2^7+2^14).
    * if the third byte of Encoded-Unsigned-Int is c3 >= 128, then the next byte c4 is needed:

      + if the fourth byte of Encoded-Unsigned-Int is c4 <= 127, then the value of Encoded-Unsigned-Int is equal to *2113664+((uint32)(c1&0x7F) | ((uint32)(c2&0x7F) << 7)) | ((uint32)(c3&0x7F) << 14)) | ((uint32)c4 << 21))* (note that 2113664 is 2^7+2^14+2^21).
      + if the fourth byte of Encoded-Unsigned-Int is c4 >= 128, then the next byte c5 is needed.

        * for nth byte:

          + if the nth byte of Encoded-Unsigned-Int is cn <= 127, then the value of Encoded-Unsigned-Int is equal to *start+((uintNN)(c1&0x7F) | ((uintNN)(c2&0x7F) << 7)) | ((uintNN)(c3&0x7F) << 14)) | ... | ((uintNN)(c<n-1>&0x7F) << (7*(n-2))))) | ((uintNN)cn << (7*(n-1))))*, where *start=2^7+2^14+...+2^(n-1)*, and uintNN is sufficient to store the result. *NB: in practice, for Encoded-Unsigned-Ints over 4 bytes, implementation is likely to be quite different from, but equivalent to, the formula given*
          + if the nth byte of Encoded-Unsigned-Int is cn >= 128, then the <n+1>th byte is needed.
 
The following table shows how many Encoded-Unsigned-Int bytes is necessary to encode ranges of Encoded-Unsigned-Int values:

+-------------------------+---------------------+------------------+------------------+
| Encoded-Unsigned-Int    | Encoded-Unsigned-Int| Fully Covers     | Result fits in   |
| Values                  | Bytes               |                  |                  |
+=========================+=====================+==================+==================+
| 0-127                   | 1                   | 7 bits           | 1 byte           |
+-------------------------+---------------------+------------------+------------------+
| 128-16 511              | 2                   | 14 bits          | 2 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 16 512-2 113 663        | 3                   | 21 bits          | 3 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 2 113 664-270 549 119   | 4                   | 28 bits          | 4 bytes          |
+-------------------------+---------------------+------------------+------------------+
| 270 549 120-            | 5                   | 35 bits          | 5 bytes          |
| 34 630 287 487          |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 34 630 287 487-         | 6                   | 42 bits          | 6 bytes          |
| 4 432 676 798 591       |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 4 432 676 798 592-      | 7                   | 49 bits          | 7 bytes          |
| 567 382 630 219 903     |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
| 567 382 630 219 904-    | 8                   | 56 bits          | 8 bytes          |
| 72 624 976 668 147 839  |                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+
|72 624 976 668 147 840-  | 9                   | 63 bits          | 8 bytes          |
|9 295 997 013 522 923 647|                     |                  |                  |
+-------------------------+---------------------+------------------+------------------+

Encoded-Signed-Int
''''''''''''''''''

Encoded-Signed-Int is an encoding for signed integers, derived from Encoded-Unsigned-Int. Encoded-Signed-Int is decoded as Encoded-Unsigned-Int first (NB: actual implementations MAY and probably SHOULD differ), and then, depending on number of bytes in the encoding (when it was treated as Encoded-Unsigned-Int), a certain constant is deducted. For example, if we need Encoded-Signed-Int, have read it as Encoded-Unsigned-Int, and got 1 byte, we need to subtract 64 to get Encoded-Signed-Int. Therefore, Encoded-Signed-Int encoding which consists out of one byte with value 64, means '0'. The following table show the way how to calculate Encoded-Signed-Int (within the table, "EUI" means "value of Encoded-Unsigned-Int"):

+---------------------+-------------------+-------------------------+
| Encoded-Unsigned-Int| Encoded-Signed-Int|Encoded-Signed-Int       |
| Bytes               |                   |Values                   |
+=====================+===================+=========================+
| 1                   | EUI - 64          | -64 to 63               |
+---------------------+-------------------+-------------------------+
| 2                   | EUI - 8256        | -8256 to 8255           |
+---------------------+-------------------+-------------------------+
| 3                   | EUI - 1056832     | -1056832 to 1056831     |
+---------------------+-------------------+-------------------------+
| 4                   | EUI - 135274560   | -135274560 to 135274559 |
+---------------------+-------------------+-------------------------+
| 5                   | EUI - 17315143744 | -17315143744 to         |
|                     |                   | 17315143743             |
+---------------------+-------------------+-------------------------+
| 6                   | EUI -             | -2216338399296 to       |
|                     | 2216338399296     | 2216338399295           |
+---------------------+-------------------+-------------------------+
| 7                   | EUI -             | -283691315109952 to     |
|                     | 283691315109952   | 283691315109951         |
+---------------------+-------------------+-------------------------+
| 8                   | EUI -             | -36312488334073920 to   |
|                     | 36312488334073920 | 36312488334073919       |
+---------------------+-------------------+-------------------------+
| 9                   |EUI -              | -4647998506761461824 to |
|                     |4647998506761461824| 4647998506761461823     |
+---------------------+-------------------+-------------------------+


Encoded-\*-Int<max=>
''''''''''''''''''''

Wherever SmartAnthill specification mentions Encoded-Unsigned-Int or Encoded-Signed-Int, it MUST specify it in the form of *Encoded-Unsigned-Int<max=...>* or *Encoded-Signed-Int<max=...>*. "max=" parameter specifies maximum number of bytes which are necessary to represent the encoded number. For example, Encoded-Unsigned-Int<max=2> specifies that the number is between 0 and 65535 (and therefore from one to three bytes may be used to encode it). The high bit of the last possible byte of Encoded-\*-Int is always 0; this ensures an option for an easy expansion in the future.

Currently supported values of "max=" parameter are from 1 to 8.

When parsing Encoded-\*-Int, if high bit in the last-possible byte is 1, then Encoded-\*-Int is considered invalid. Handling of invalid Encoded-\*-Ints SHOULD be specified in the appropriate place of documentation.

SmartAnthill Endianness
^^^^^^^^^^^^^^^^^^^^^^^

In most cases, SmartAnthill Protocol Stack uses SmartAnthill Encoded-\*-Int<max=...> to encode integers. However, there are some cases where we need an exact number of bytes, and have no idea about their statistical distribution. In such cases, using Encoded-\*-Int<> would be a waste. 

In such cases, SmartAnthill uses **SmartAnthill Endianness**, which is **LITTLE-ENDIAN**.

*Rationale for using LITTLE-ENDIAN encoding (rather than "network byte order" which is traditionally big-endian) is based on the observation that the most resource-constrained MPUs out of target group (namely PIC and AVR8), are little-endian. For them, the difference of not doing conversion between protocol-order and MPU-order might be important; as the other MPUs are not that much constrained, we don't expect the cost of conversion to be significant. In other words, this LITTLE-ENDIAN decision to favours poorer-resource MPUs at the cost of richer-resource MPUs.*

SmartAnthill Bitfields
^^^^^^^^^^^^^^^^^^^^^^

In some cases, SmartAnthill Protocols use bitfields; in such cases: 

* bitfields MUST use 1-byte, 2-byte, Encoded-Unsigned-Int<max=>, or Encoded-Signed-Int<max=> field as a 'substrate'. 'Bitfield Substrate' is composed/parsed as an ordinary field, which is encoded using appropriate encodings described in this document.
* as soon as 'substrate' is parsed, it is treated as an integer, out of which specific bits can be used; these bits are specified as [3] (specifying that single bit #3 is used), or [2..4] (specifying that bits from 2 to 4 - inclusive - are used)
* if 'substrate' is an Encoded-Unsigned-Int field, then one of bitfields MAY be specified as [2..] - specifying that all the bits from 2 to the highest available one, are used for the bitfield.
* if 'substrate' is an Encoded-Signed-Int field, then one of bitfields MAY be specified as [2..] - specifying that all the bits from 2 to the highest available one, are used for the bitfield; in this example, the bitfield in question MUST be calculated as `substrate>>1`, where substrate is treated as signed (i.e. '>>' operator works extending sign bit).

SmartAnthill Half-Float
^^^^^^^^^^^^^^^^^^^^^^^

Some SmartAnthill commands use 'Half-Float' data as described here: http://en.wikipedia.org/wiki/Half-precision_floating-point_format . SmartAnthill serializes such data as 2-byte substrate (encoded according to SmartAnthill Endianness), then considering Sign-Bit bitfield as bit [15], Exponent bitfield as bits [10..14], and Fraction bitfield as bits [0..9].

Layering remarks
----------------

SACCP and "packet chains"
^^^^^^^^^^^^^^^^^^^^^^^^^

SACCP is somewhat unusual for an application-level protocol in a sense that SACCP needs to have some knowledge about "packet chains" which are implicitly related to retransmission correctness. This is a conscious design choice of SACCP (and SAGDP) which has been made in face of extremely constrained (and unusual for conventional communication) environments which SmartAnthill protocol stack needs to support. It should also be noted that while some such details are indeed exposed to SACCP, they are formalized as a clear set of “rules of engagement” to be obeyed. As long as these “rules of engagement” are complied with, SACCP does not need to care about retransmission correctness (though the rationale for “rules of engagement” is still provided by retransmission correctness).

SASP below SAGDP
^^^^^^^^^^^^^^^^

It is somewhat unusual to have encryption layer (SASP) "below" transport/session layer (SAGDP). This is a conscious design choice of SASP/SAGDP. In particular, it allows to:

* rely that all the packets reaching SAGDP layer, are already authenticated; this allows (at the cost of the authenticating potentially malicious packets) to:

  + avoid attacks such as malicious RST sent to disrupt logical connection (TODO: check)
  + avoid attacks similar to "SYN flood" attacks

* implement "Trusted Router" nodes in a simple manner (without implementing SAGDP on the router).

