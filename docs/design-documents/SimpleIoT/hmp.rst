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

.. _siot_hmp:

SimpleIoT Heterogeneous Mesh Protocol (SimpleIoT/HMP)
=====================================================

:Version:   0.1.7

*NB: this document relies on certain terms and concepts introduced in* :ref:`siot` *document, please make sure to read it before proceeding.*

SimpleIoT/HMP is a part of SimpleIoT protocol stack. It belongs to Level 3 of OSI/ISO Network Model, and is responsible for routing packets within SimpleIoT mesh network.

SimpleIoT mesh network is a heterogeneous network. In particular, on the way from SimpleIoT Client to SimpleIoT Device a packet may traverse different bus types (including all supported types of wired and wireless buses); the same stands for the packet going in the opposite direction.

SimpleIoT/HMP is optimized based on the following assumptions:

* SimpleIoT/HMP relies on all communications being between Central Controller and Device (no Device-to-Device communications); no other communications are currently supported
* SimpleIoT/HMP aims to optimize "last mile" traffic (between last Retransmitting Device and target Device) while paying less attention to Central-Controller-to-Retransmitting-Device and Retransmitting-Device-to-Retransmitting-Device traffic. This is based on the assumption that the Retransmitting Devices usually have significantly less power restrictions (for example, are mains-powered rather than battery-powered).
* SimpleIoT/HMP combines data with route requests
* SimpleIoT/HMP allows to send "urgent" data packets, which sacrifice traffic and energy consumption for the best possible delivery speed
* SimpleIoT/HMP relies on pre-existence of Routing Tables (see below) on all relevant Retransmitting Nodes. Communicating Routing Tables MAY be implemented over the upper-layer protocol such as SimpleIoT/CCP

  + This is done because of sensitivity of Routing Tables; with upper-layer protocol, Routing Tables can be communicated securely
  + It doesn't create a chicken-and-egg problem, as SimpleIoT/HMP provides a way to reach any reachable Retransmitting Node without a Routing Table on it; as soon as Retransmitting Node is reachable via SimpleIoT/HMP, upper-layer protocol such as SimpleIoT/CCP can be used to create/update Routing Table on the Retransmitting Node.
  + Technically, updating Routing Tables is not a part of SimpleIoT/HMP; however, a protocol of updating Routing Tables over CCP_PHY_AND_ROUTING_DATA messages is provided below as an example.

* SimpleIoT/HMP relies on upper-layer protocol (such as SimpleIoT/GDP) to send retransmits in case if packet has not been delivered, and to provide SimpleIoT/HMP with an information about retransmit number (i.e., original packet having retransmit-number=0, first retransmit having retransmit-number=1, and so on).
* SimpleIoT/HMP relies on upper-layer protocol (such as SimpleIoT/GDP) to provide information if the Device on the other side is required to have it's transmitter on for upper-layer protocol purposes. For SimpleIoT/GDP, there are states which do guarantee this (in fact, it stands in almost all SimpleIoT/GDP states except for IDLE).

SimpleIoT/HMP has the following types of actors: Root (normally implemented by Central Controller), Retransmitting Device, and non-Retransmitting Device. All these actors are collectively named Nodes.

Underlying Protocol Requirements
--------------------------------

SimpleIoT/HMP underlying protocol (normally one of SimpleIoT/DLP-\* protocols), MUST support the following operations:

* bus broadcast (addressed to all the Devices on the bus)
* bus multi-cast (addressed to a list of the Devices on the bus)
* bus uni-cast

NB: these operations MAY be implemented using only bus broadcast, without any additional intra-bus addressing information; all HMP packets have sufficient information to ensure further processing of HMP packets without underlying protocol addressing information. If some information within HMP packet becomes redundant given underlying protocol's addressing information, underlying protocol MAY compress HMP packet when transmitting it, by re-using underlying-protocol information when compressing HMP packet; however, as HMP addresses in normal (post-pairing) communication are usually very short anyway, such compression is not likely to bring substantial benefits.

All SimpleIoT Devices SHOULD, and all SimpleIoT Retransmitting Devices MUST implement some kind of collision avoidance (at least CSMA/CA, a.k.a. "listen before talk with random delay").

SCRAMBLING and Underlying Protocol Error Correction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

HMP packets are usually SCRAMBLED, and after SCRAMBLING are transmitted over some of SimpleIoT/DLP-\* protocols. 

SADLP-\* protocols SHOULD allow for gradual error correction, starting from the beginning of the packet. Even if the packet cannot be error-corrected completely, information in the first part of the header MAY be of value, and SHOULD be passed to upper layers. SCRAMBLING procedure SHOULD allow for partial descrambling (to the extent possible), and SHOULD return partially descrambled packets back to SimpleIoT/HMP. It will allow SimpleIoT/HMP to get "partially correct" packets, which are to be used as described below, to improve certain SimpleIoT/HMP characteristics. SimpleIoT/HMP uses headers of "partially correct" packets in "promiscuous mode" operations, and in some other cases referred to as "partially correct packet".

Promiscuous Mode Operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Wherever possible (in particular, for all kinds of wireless communications unless explicitly prohibited by underlying standard), SimpleIoT Retransmitting Devices SHOULD listen the network in promiscuous mode; this doesn't affect security, but provides valuable header information and speeds up message delivery and recovery in certain practical cases.

SimpleIoT Retransmitting Devices
--------------------------------

Some SimpleIoT Devices are intended to be "SimpleIoT Retransmitting Devices". "SimpleIoT Retransmitting Device" has one or more transmitters. Transmitters on SimpleIoT Retransmitting Devices MUST be always-on (except for possible intermittent turning off if described in a corresponding SimpleIoT/DLP\* document); turning off transmitter is NOT allowed for SimpleIoT Retransmitting Devices. That is, if MCUSLEEP instruction is executed on a SimpleIoT Retransmitting Device, it simply pauses executing a program, without turning transmitter off (TODO: add to Zepto VM). Normally, SimpleIoT Retransmitting Devices are mains-powered, or are using large batteries. SimpleIoT Protocol Stack (specifically SimpleIoT/HMP) on SimpleIoT Retransmitting Devices requires more resources (specifically RAM) than non-Retransmitting Devices.

Highly mobile Devices SHOULD NOT be Retransmitting Devices. Building a reliable network out of highly mobile is problematic from the outset (and right impossible if these movements are not synchronized). Therefore, SimpleIoT/HMP assumes that Retransmitting Devices are moved rarely, and is optimized for rarely-moving Retransmitting Devices. While SimpleIoT/HMP does recover from moving one or even all Retransmitting Devices, this scenario is not optimized and recovery from it may take significant time.

Routing Tables
--------------

Each Retransmitting Device, after pairing, MUST keep a Routing Table. Routing Table consists of two lists: (a) Links list, with each entry being (LINK-ID,BUS-ID,INTRA-BUS-ID,NEXT-HOP-ACKS,LINK-DELAY-UNIT,LINK-DELAY,LINK-DELAY-ERROR) tuple, and (b) Routes list, with each entry being (TARGET-ID,LINK-ID). LINK-ID is an intra-Routing-Table id, used to map routes into links.

Each entry in Routes list has semantics of "where to route packet addressed to TARGET-ID". In Links list, INTRA-BUS-ID=NULL means that the entry is for an incoming link. Incoming link entries are relatiely rare, and are used to specify LINK-DELAYs.

NEXT-HOP-ACKS is a flag which is set if the nearest hop (over (BUS-ID,INTRA-BUS-ID)) is known to be able not only to receive packets, but to send ACKs back; in general, NEXT-HOP-ACKS cannot be calculated based only on bus type, and may change for the same link during system operation; SimpleIoT/HMP is built to try using links with NEXT-HOP-ACKS as much as possible, but MAY use links without NEXT-HOP-ACKS if there are no alternatives.

TODO: size reporting to Root (as # of unspecified 'storage units', plus sizes of Links entry and Routes entry expressed in the same 'storage units'). 

Routing Tables SHOULD be stored in a 'canonical' way (Links list ordered from lower LINK-IDs to higher ones, Routes list ordered from lower TARGET-IDs to higher ones; duplicate entries for the same LINK-ID are prohibited, for the same TARGET-ID are currently prohibited); this is necessary to simplify calculations of the Routing Table checksums. TODO: specify Routing-Table-Checksum calculation

On non-Retransmitting Devices, Routing Table is rudimentary: it contains only one Link (LINK-ID=0,BUS-ID,INTRA-BUS-ID,...) and only one Route (TARGET-ID=0,LINK-ID=0). Moreover, on non-Retransmitting Devices Routing Table is OPTIONAL; if non-Retransmitting Device does not keep Routing Table - it MUST be reflected in a TODO CAPABILITIES flag during "pairing"; in this case Root MUST send requests to such devices specifying TODO header extension (which contains BUS-ID,INTRA-BUS-ID for the first hop back from target Device).

All Routing Tables on both Retransmitting and non-Retransmitting Devices are essentially (usually partial) replicas of "Master Routing Tables" which are kept on Root. It is a responsibility of Root to maintain Routing Tables for all the Devices (both Retransmitting and non-Retransmitting); it is up to Root which entries to store in each Routing Table. In some cases, Routing Table might need to be truncated; in this case, it is responsibility of Root to use VIA field in Target-Address (see below) to ensure that the packet can be routed given the Routing Tables present. In any case, Routing Table MUST be able to contain at least one entry, with TARGET-ID=0 (Root). This guarantees that path to Root can always be found without VIA field.

In addition, on Retransmitting Devices the following parameters are kept (and updated by Root): MAX-TTL, FORWARD-TO-SANTA-DELAY-UNIT, FORWARD-TO-SANTA-DELAY, MAX-FORWARD-TO-SANTA-DELAY (using same units as FORWARD-TO-SANTA-DELAY), NODE-MAX-RANDOM-DELAY-UNIT, and NODE-MAX-RANDOM-DELAY. MAX-FORWARD-TO-SANTA-DELAY indicates maximum "forward to santa" delay for all Retransmitting Devices in the PAN.

TODO: no mobile non-Retransmitting (TODO reporting 'mobile' in pairing CAPABILITIES, plus heuristics), priorities (low->high): non-Retransmitting, Retransmitting.

Broken Routing Tables
^^^^^^^^^^^^^^^^^^^^^

Despite that Routing Tables are updated only by authenticated upper-layer messages, SimpleIoT/HMP does recognize that Routing Tables may become broken during operation. To deal with it, two separate procedures are used. One such procedure is intended for destination Devices (either Retransmitting or non-Retransmitting), and is described within "Unicast" section below. Another procedure is intended for Retransmitting Devices, and is described in "Acknowledged Unicast" section below.

Communicating Routing Table Information over SimpleIoT/CCP
----------------------------------------------------------

As described above, SimpleIoT/HMP relies on Routing Table information being available on all relevant Retransmitting Nodes. To ensure that this information is transmitted in secure manner, it SHOULD be transmitted by an upper-layer secure (and acknowledged-delivery) protocol such as SimpleIoT/CCP. As described above, this doesn't create a chicken-and-egg problem, as each Retransmitting Node can be accessed via SimpleIoT/HMP regardless of Routing Tables present (or even badly broken) on the Retransmitting Node in question; and as soon as Retransmitting Node can be accessed via SimpleIoT/HMP - upper-layer protocol such as SimpleIoT/CCP can be used to update Routing Table on the Retransmitting Node. 

Technically, protocol for communicating Routing Table information is not a part of SimpleIoT/HMP. However, in this section we provide an example implementation of such protocol over CCP_PHY_AND_ROUTING_DATA packets.

CCP_PHY_AND_ROUTING_DATA supports the following packets:

Route-Update-Request: **\| FLAGS \| OPTIONAL-EXTRA-HEADERS \| OPTIONAL-ORIGINAL-RT-CHECKSUM \| OPTIONAL-MAX-TTL \| OPTIONAL-FORWARD-TO-SANTA-DELAY-UNIT \| OPTIONAL-FORWARD-TO-SANTA-DELAY \| OPTIONAL-MAX-FORWARD-TO-SANTA-DELAY \| OPTIONAL-MAX-NODE-RANDOM-DELAY-UNIT \| OPTIONAL-MAX-NODE-RANDOM-DELAY \| MODIFICATIONS-LIST \| RESULTING-RT-CHECKSUM \|**

where FLAGS is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] being DISCARD-RT-FIRST (indicating that before processing MODIFICATIONS-LIST, the whole Routing Table must be discarded), bit[1] being UPDATE-MAX-TTL flag, bit[2] being UPDATE-FORWARD-TO-SANTA-DELAY flag, bit[3] being UPDATE-MAX-NODE-RANDOM-DELAY flag, and bits[4..] reserved (MUST be zeros); OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT is set, and is described above; Target-Address is the Target-Address field; OPTIONAL-ORIGINAL-RT-CHECKSUM is present only if DISCARD-RT-FIRST flag is not set; OPTIONAL-ORIGINAL-RT-CHECKSUM is a Routing-Table-Checksum, specifying Routing Table checksum before the change is applied; if OPTIONAL-ORIGINAL-RT-CHECKSUM doesn't match to that of the Routing Table - it is TODO Routing-Error; OPTIONAL-MAX-TTL is present only if UPDATE-MAX-TTL flag is present, and is a 1-byte field, OPTIONAL-FORWARD-TO-SANTA-DELAY-UNIT, OPTIONAL-FORWARD-TO-SANTA-DELAY, and OPTIONAL-MAX-FORWARD-TO-SANTA-DELAY are present only if UPDATE-FORWARD-TO-SANTA-DELAY flag is present, and all are Encoded-Signed-Int<max=2> fields, OPTIONAL-MAX-NODE-RANDOM-DELAY-UNIT and OPTIONAL-MAX-NODE-RANDOM-DELAY are present only if UPDATE-MAX-NODE-RANDOM-DELAY flag is present, and both are Encoded-Unsigned-Int<max=2> fields, MODIFICATIONS-LIST described below; RESULTING-RT-CHECKSUM is a Routing-Table-Checksum, specifying Routing Table Checksum after the change has been applied (if RESULTING-RT-CHECKSUM doesn't match - it is TODO Routing-Error). 

Route-Update-Request is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x0 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

MODIFICATIONS-LIST consists of entries, where each entry is one of the following: 

* **\| ADD-OR-MODIFY-LINK-ENTRY-AND-LINK-ID \| BUS-ID \| NEXT-HOP-ACKS-AND-INTRA-BUS-ID-PLUS-1 \| OPTIONAL-LINK-DELAY-UNIT \| OPTIONAL-LINK-DELAY \| OPTIONAL-LINK-DELAY-ERROR \|**

  where ADD-OR-MODIFY-LINK-ENTRY-AND-LINK-ID is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] marks the end of MODIFICATIONS-LIST, bits[1..2] equal to a 2-bit constant ADD_OR_MODIFY_LINK_ENTRY, bit[3] being LINK-DELAY-PRESENT flag, and bits[4..] equal to LINK-ID; BUS-ID is an Encoded-Unsigned-Int<max=2> field, NEXT-HOP-ACKS-AND-INTRA-BUS-ID is an Encoded-Unsigned-Int<max=4> bitfield substrate, with bit[0] being a NEXT-HOP-ACKS flag for the Routing Table Entry, and bits[1..] representing INTRA-BUS-ID-PLUS-1 (INTRA-BUS-ID-PLUS-1 == 0 means that INTRA-BUS-ID==NULL, and therefore that the link entry is an incoming link entry; otherwise, `INTRA-BUS-ID = INTRA-BUS-ID-PLUS-1 - 1`); OPTIONAL-LINK-DELAY-UNIT, OPTIONAL-LINK-DELAY, and OPTIONAL-LINK-DELAY-ERROR are present only if LINK-DELAY-PRESENT flag is set, and are Encoded-Unsigned-Int<max=2> fields. NB: by default, link delays are not set by Root, and are set based on device's internal per-bus settings.

* **\| DELETE-LINK-ENTRY-AND-LINK-ID \|**

  where DELETE-LINK-ENTRY-AND-LINK-ID is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] marks the end of MODIFICATIONS-LIST, bits[1..2] equal to a 2-bit constant DELETE_LINK_ENTRY, and bits[3..] equal to LINK-ID.

* **\| ADD-OR-MODIFY-ROUTE-ENTRY-AND-LINK-ID \| TARGET-ID \|**

  where ADD-OR-MODIFY-ROUTE-ENTRY-AND-LINK-ID is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] marks the end of MODIFICATIONS-LIST, bits[1..2] equal to a 2-bit constant ADD_OR_MODIFY_ROUTE_ENTRY, and bits[3..] equal to LINK-ID; TARGET-ID is an Encoded-Unsigned-Int<max=2> field.

* **\| DELETE-ROUTE-ENTRY-AND-TARGET-ID \|**

  where DELETE-ROUTE-ENTRY-AND-TARGET-ID is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] marks the end of MODIFICATIONS-LIST, bits[1..2] equal to a 2-bit constant DELETE_ROUTE_ENTRY, and bits[3..] equal to TARGET-ID. Note that DELETE-ROUTE-ENTRY-AND-TARGET-ID is the only MODIFICATIONS-LIST entry first field which includes TARGET-ID rather than LINK-ID.

Route-Update-Request packets always go from Root to Device. Route-Update-Request MAY be sent either to Retransmitting or to non-Retransmitting Device; however (as with any SimpleIoT/CCP packet), if sending it to a non-Retransmitting Device, Root MUST be sure that non-Retransmitting Device has it's transmitter turned on (because upper-layer protocol state guarantees it).

Route-Update-Response: **\| ERROR-CODE \|** TODO: more error info if any

where ERROR-CODE is an Encoded-Unsigned-Int<max=1> field, containing error code. ERROR-CODE = 0 means that Route-Update-Request has been completed successfully.

Route-Update-Response is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x0 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

Communicating PHY Information over SimpleIoT/CCP
------------------------------------------------

Some of SimpleIoT/DLP-\* protocols (as described in corresponding SimpleIoT/DLP-\* document) MAY need to communicate information to Central Controller (for example, to calculate optimums using quite complicated methods).

This is done via the following packets:

PHY-Data-Request: **\| ID-OF-SADLP \| DLP-DEPENDENT-PAYLOAD \|**
where ID-OF-SADLP is an Encoded-Unsigned-Int<max=2> field, specified in respective SimpleIoT/DLP-\* document. TODO: list of all IDs in one place to avoid potential for collisions.

PHY-Data-Request is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x1 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

PHY-Data-Response: **\| DLP-DEPENDENT-PAYLOAD \|**

PHY-Data-Response is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x1 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

PHY-Data-Ready-Request: **\|** (empty)

PHY-Data-Ready-Request is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x2 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

PHY-Data-Ready-Response: **\|** (empty)

PHY-Data-Ready-Response is always accompanied with SimpleIoT/CCP "additional bits" equal to 0x2 (see :ref:`siot_ccp` for details on CCP_PHY_AND_ROUTING_DATA "additional bits").

To indicate that PHY-level tuning is completed, Device sends PHY-Data-Ready-Response (sic!); this happens at the point specified in respective SimpleIoT/DLP-\* document. In response, Root sends PHY-Data-Ready-Request (sic!). 

Addressing
----------

SimpleIoT/HMP supports two ways of addressing devices: non-paired and paired. 

Non-paired addressing is used for temporary addressing Devices which are not "paired" with SimpleIoT Root (yet). Non-paired addressing is used ONLY during "Pairing" process, as described in :ref:`siot_pairing` document. As soon as "pairing" is completed, Device obtains it's own HMP-NODE-ID (TODO: add to pairing document), and all further communications with Device is performed using  "paired" addressing. Non-paired addressing is a triplet (NODE-ID,BUS-ID,INTRA-BUS-ID).

Paired addressing is used for addressing Devices which has already been "paired". It is always one single item HMP-NODE-ID. Root always has HMP-NODE-ID=0. 

HMP Checksums
-------------

To validate integrity of HMP headers, and of the whole HMP packets, HMP-CHECKSUM is used. 

HMP-CHECKSUM is defined as a Fletcher-16 checksum, as described in https://en.wikipedia.org/wiki/Fletcher%27s_checksum (using modulo 255), stored using "SimpleIoT Endianness".

Whenever the packet has both header and body, SimpleIoT/HMP uses two HMP-CHECKSUMs: first checksum (referred to as HEADER-CHECKSUM) encompasses only header (i.e. everything before the first checksum), second HMP-CHECKSUM (referred to as FULL-CHECKSUM) is located at the very end and encompasses header+first_checksum+body (i.e. everything before the second checksum).


DELAYs and DELAY-UNITs
----------------------

Whenever delay (or more generally - time interval) needs to be calculated, it is always represented as two fields: DELAY itself and corresponding DELAY-UNIT. 

To calculate delay for specific DELAY and DELAY-UNIT, the following formula is used (the formula as written is assumed to be in floating-point; other equivalent implementations are possible depending in particular on timer resolution for specific Device): `delay = 1 millisecond * DELAY * (2^DELAY_UNIT)`; that is, DELAY-UNIT=0 and DELAY=1 means 1 millisecond, DELAY-UNIT=1 and DELAY=1 means 2 milliseconds, and DELAY-UNIT =-2 and DELAY=1 means 0.25 milliseconds. 

Recovery Philosophy
-------------------

Recovery from route changes/failures is vital for any mesh protocol. SimpleIoT/HMP does it as follows:

* by default, most of the transfers are not acknowledged at SimpleIoT/HMP level (go as Hmp-Unicast-Data-Packet without ACKNOWLEDGED-DELIVERY flag)
* however, upper-layer protocol (normally SimpleIoT/GDP) issues it's own retransmits and passes retransmit number to SimpleIoT/HMP
* on retransmit #TODO, SimpleIoT/HMP switches ACKNOWLEDGED-DELIVERY flag on
* when ACKNOWLEDGED-DELIVERY flag is set, SimpleIoT/HMP uses 'Acknowledged Uni-Cast' mode described below
* if 'Acknowledged Uni-Cast' fails for M times (as described below), link failure is assumed
* link failure (as described above) is reported to the Root, so it can initiate route discovery to the node on the other side of the failed link (using Hmp-From-Santa-Data-Packet)

  + if link failure is detected from the side of the link which is close to Root, link failure reporting is done by sending Routing-Error (which always come in ACKNOWLEDGED-DELIVERY mode) back to Root
  + if link failure is detected from the side of the link which is far from Root, link failure reporting is done by broadcasting Hmp-To-Santa-Data-Or-Error-Packet, which is then converted into Hmp-Forward-To-Santa-Data-Or-Error-Packet (which is always sent in ACKNOWLEDGED-DELIVERY mode) by all Retransmitting Devices which have received it.

Storm Avoidance
---------------

To reduce number of induced collisions during broadcasts, a.k.a. "request storm" and "reply storm" (NB: avoiding "storms" is important even when CSMA/CA is present, because CSMA/CA provides only probabilistic success), SimpleIoT/HMP supports two mechanisms: explicit time-based collision avoidance, and random-delay-based storm avoidance. 

Explicit Time-Based Storm Avoidance and Collision Domains
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SimpleIoT/HMP explicit time-based collision avoidance works as follows:

* to avoid "request storm": when performing a 'network flood' (using Hmp-From-Santa-Data-Packet), Root MAY specify explicit time delays for each node. 
* to avoid "reply storm": Root MAY specify FORWARD-TO-SANTA-DELAY-\* parameters; whenever a Hmp-To-Santa-Data-Or-Error-Packet (these are essentially sent as "anybody who can hear this, forward it to Root"), is received by Retransmitting Node, each of receiving Retransmitting Nodes waits according to FORWARD-TO-SANTA-DELAY before retransmitting.
* In addition (to avoid "storms" in general), each HMP packet, MAY have a 'Collision-Domain' restrictions (i.e. "from t0-from-now to t1-from-now, don't transmit on Collision-Domain #CD); these restrictions specify . **Retransmitting Devices SHOULD monitor Collision-Domain headers in promiscuous mode and work accordingly, even if the packet is not addressed to this Retransmitting Device**.

Random-delay-based Storm Avoidance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If explicit time-based collision avoidance is not used, Retransmitting Devices MUST use random delays (based on NODE-MAX-RANDOM-DELAY-UNIT and NODE-MAX-RANDOM-DELAY) as specified below.

Target-Address, Multiple-Target-Addresses, and Multiple-Target-Addresses-With-Extra-Data
----------------------------------------------------------------------------------------

Target-Address allows to store either paired-address, or non-paired address. Target-Address is encoded as 

**\| FLAG-AND-NODE-ID \| OPTIONAL-VIA-OR-INTRA-BUS-SIZE-AND-BUS-ID \| ... \| OPTIONAL-VIA-OR-INTRA-BUS-SIZE-AND-BUS-ID \| OPTIONAL-CUSTOM-INTRA-BUS-SIZE \| OPTIONAL-INTRA-BUS-ID \|**

where FLAG-AND-NODE-ID-OR-BUS-ID is an Encoded-Unsigned-Int<max=2> bitfield substrate, where bit[0] is EXTRA_DATA_FOLLOWS flag, and bits[1..] are NODE-ID.

OPTIONAL-VIA-OR-INTRA-BUS-SIZE-AND-BUS-ID is present only if EXTRA_DATA_FOLLOWS is set, and is an Encoded-Unsigned-Int<max=2> bitfield substrate, where bit[0] represents IS_NONPAIRED_ADDRESS flag, and the rest of the bits depend on bit[0]. If IS_NONPAIRED_ADDRESS flag is not set, then bits[1..] represent VIA field (encoded as `NODE-ID+1`); if VIA field is -1 (because bits[1..] are zero), then no further extra data fields are present. If IS_NONPAIRED_ADDRESS flag is set, then bits[1..3] represent INTRA-BUS-SIZE (with value 0x7 interpreted in a special way, specifying that INTRA-BUS-SIZE is 'custom'), and bits [4..] represent BUS-ID. If IS_NONPAIRED_ADDRESS flag is not set, and VIA field in it is >=0, it means that another OPTIONAL-VIA-INTRA-BUS-SIZE-AND-BUS-ID field is present, which is interpreted as above. OPTIONAL-VIA-INTRA-BUS-SIZE-AND-BUS-ID with either IS_NONPAIRED_ADDRESS set, or with VIA field equal to -1, denote the end of the list.

OPTIONAL-CUSTOM-INTRA-BUS-SIZE is present only if OPTIONAL-VIA-OR-INTRA-BUS-SIZE-AND-BUS-ID is present, and flag IS_NONPAIRED_ADDRESS is set, and INTRA-BUS-SIZE field has value 'custom'; OPTIONAL-INTRA-BUS-ID is present only if OPTIONAL-VIA-OR-INTRA-BUS-SIZE-AND-BUS-ID is present, and has INTRA-BUS-SIZE (calculated from OPTIONAL-INTRA-BUS-SIZE-AND-BUS-ID and OPTIONAL-CUSTOM-INTRA-BUS-SIZE) size.

Multiple-Target-Addresses is essentially a multi-cast address. It is encoded as a list of items, where each item is similar to an Target-Address field, with the following changes: 

* for list entries, within FLAG-AND-NODE-ID field it is `NODE-ID + 1` which is stored (instead of simple NODE-ID for single Target-Address). This change does not affect VIA fields.
* to denote the end of Multiple-Target-Addresses list, FLAG-AND-NODE-ID field with EXTRA_DATA_FOLLOWS=0 and NODE-ID=0, is used
* value of FLAG-AND-NODE-ID field with EXTRA_DATA_FOLLOWS=1 and NODE-ID=0, is prohibited (reserved)

Multiple-Target-Addresses-With-Extra-Data is the same as Multiple-Target-Addresses, but each item (except for the last one, where NODE-ID=0), additionally contains some extra data (which is specified whenever Multiple-Target-Addresses-With-Extra-Data is mentioned). For example, if we're speaking about "Multiple-Target-Addresses-With-Extra-Data, where Extra-Data is 1-byte field", it means that each item of the list (except for the last one) will have both Target-Address field (with changes described in Multiple-Target-Addresses), and 1-byte field of extra data.

Time-To-Live
------------

Time-To-Live (TTL) is a field which is intended to address misconfigured/inconsistent Routing Tables. TTL is set to certain value (default 4) whenever the packet is sent, and is decremented by each Node which retransmits the packet. TTL=0 is valid, but TTL < 0 is not; whenever the packet needs to be retransmitted and it would cause TTL to become < 0 - the packet is dropped (with a Routing-Error, see below).

During normal operation, it SHOULD NOT occur. Whenever the packet is dropped because TTL is down to zero (except for Routing-Error HMP packets), it MUST cause a TODO Routing-Error to be sent to Root.

Uni-Cast Processing
-------------------

Whenever a Uni-Cast packet (the one with a Target-Address field) is received by Retransmitting Device, the procedure is the following:

* check if the Target-Address is intended for the Retransmitting Device

  + if it is - process the packet locally and don't process further

* if packet TTL is already equal to 0 - drop the packet and send Routing-Error to the Root (see Time-To-Live section above for details)
* decrement packet TTL
* using Routing Table, find next hop for the Target-Address

  + if next hop cannot be found for the Target-Address itself, but Target-Address contains VIA field(s) - try to find next hop based on each of VIA fields
  + if next hop cannot be found using Target-Address and all VIA field(s) - drop the packet and send TODO Routing-Error to the Root

* if any of VIA fields in the Target-Address is the same as the next hop - remove all such VIA fields from the Target-Address
* find bus for the next hop and send modified packet (see on TTL and VIA modifications above) over this bus

Processing on Destination and Broken Routing Table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As described above, SimpleIoT/HMP does recognize that Routing Tables may become broken during operation. On a destination Device, whenever Device attempts retransmit #TODO of the message, Device sends it as a Hmp-To-Santa message, ignoring local Routing Table completely; TODO: add optional-header with RT-CHECKSUM for Hmp-To-Santa messages?


Acknowledged Uni-Cast
^^^^^^^^^^^^^^^^^^^^^

As described in detail below, Hmp-Unicast-Data-Packets, except for Hmp-Unicast-Data-Packets without ACKNOWLEDGED-DELIVERY flag and Hmp-Loop-Ack-Packet, are sent in 'Acknowledged Uni-Cast' mode. 

Processing by Retransmitting Devices
''''''''''''''''''''''''''''''''''''

If packet is to be delivered to the next hop in 'Acknowledged' mode by Retransmitting Device, it is processed in the following manner:

If the packet already has LOOP-ACK extra header (see below), and next hop has NEXT-HOP-ACKS flag set in the Routing Table, then Retransmitting Device:

* sends Hmp-Loop-Ack-Packet (see below) back to the requestor specified in LOOP-ACK extra header 
* removes LOOP-ACK extra header
* continues processing as specified below

If the next hop has NEXT-HOP-ACKS flag set in the Routing Table, after sending the packet, timer is set and the packet is sent using "uni-cast" bus mechanism. If timer expires (or Node receives relevant Hmp-Ack-Nack-Packet with IS-NACK flag set), SimpleIoT/HMP retries it for 5 times (with exponentially increasing timeouts - TODO); if all 5 attempts fail - it is treated as 'Routing-Error'. In particular:

* if the packet has Root as Target-Address: 

  + packet Hmp-To-Santa-Data-Or-Error-Packet containing TBD Routing-Error as PAYLOAD (and with IS_ERROR flag set) is broadcasted
  + if possible, the packet which wasn't delivered, SHOULD be preserved (**TODO: what to do if it cannot be?**), and retransmitted as soon as route to the Root is restored

* if the packet has anything except for Root as Target-Address (and therefore is coming from Root):

  + packet Hmp-Routing-Error containing TBD Routing-Error is sent (towards Root)
  + to deal with potentially broken Routing Table on this Retransmitting Device, this Hmp-Routing-Error packet MUST contain TODO optional-header with RT-Checksum
  + the packet which wasn't delivered, doesn't need to be preserved (TODO: identify packet which has been lost within Routing-Error)

If the packet doesn't have LOOP-ACK extra header, and next hop doesn't have NEXT-HOP-ACKS flag set in the Routing Table, then Retransmitting Device:

* adds LOOP-ACK extra header (which is described below) to the packet (if it is not already present)
* sends modified packet using "bus unicast" operation
* and sets timer to TODO

  + if the sender doesn't receive Hmp-Loop-Ack-Packet until timer expires - it retransmits the packet at SimpleIoT/HMP level. 
  
    - if such attempts don't succeed for 5 (TODO) times (with exponentially increasing timeouts - TODO) - it is treated as 'Routing-Error' (the same way as described above, depending on packet having Root as a Target-Address).

If the packet already has LOOP-ACK extra header, and next hop doesn't have NEXT-HOP-ACKS flag set in the Routing Table, then Retransmitting Device:

* keeps LOOP-ACK extra header
* sends packet using "bus unicast" operation
* doesn't set any timers

LOOP-ACK on Destination
'''''''''''''''''''''''

If packet with LOOP-ACK extra header is received by destination Device, destination Device MUST send Hmp-Loop-Ack-Packet back to the node specified in LOOP-ACK extra header. If destination Device is a non-Retransmitting Device, it will send Hmp-Loop-Ack-Packet with Target-Address specified in LOOP-ACK, but to the next hop specified in Root's Routing Table entry. TODO: is it possible that Device doesn't have a route to Root yet? 

LOOP-ACK and Routing
''''''''''''''''''''

As LOOP-ACK currently doesn't support VIA routing, it means that Root MUST ensure that all the nodes on the "loop" route already know the routes without VIA fields; it applies both to the route from the loop beginning to the loop end, and back from the loop end to the loop beginning (as for request-response cycle, LOOP-ACKs go both directions). When speaking about 'back from the loop end to the loop beginning', it MUST be taken into account that, as specified above, non-Retransmitting Device will send a Hmp-Loop-Ack-Packet in the direction of the Root (but with Target-Address equal to the address from LOOP-ACK extra header), so there MUST be an already-defined route from this next-hop-in-direction-of-Root to the loop beginning.

"From-Santa" Packet Processing
------------------------------

Whenever a From-Santa packet (see below) is processed by a Retransmitting Device, the procedure is the following:

* check if one of addresses within Target-Address is intended for the Retransmitting Device (TODO: if multiple addresses match the Retransmitting Device - it is a TODO Routing-Error, which should never happen)

  + if it is - process the packet as terminating device (as described in more details while discussing Hmp-From-Santa-Data-Packet below) and do not process any further

* if packet TTL is already equal to 0 - drop the packet and send Routing-Error to the Root (see Time-To-Live section above for details)
* decrement packet TTL
* using Routing Table, find all retransmitters in the MULTIPLE-RETRANSMITTING-ADDRESSES for which routes are known, and exclude the rest from consideration; group found retransmitters according to Bus IDs in their routes; for each group of retransmitters create a new packet with MULTIPLE-RETRANSMITTING-ADDRESSES consisting of retransmitters from the group, and send the packet using respective BUS-ID
* if the retransmitter is not listed in the MULTIPLE-RETRANSMITTING-ADDRESSES, stop 
* if all bus types in the bus-type-list were used while sending packets during above steps (if any), stop
* for each remaining bus type prepare and send a packet with the same target list and empty retransmitter list.

    - if the bus supports multi-casting - send the modified packet using multi-cast bus addressing over the bus.
    - otherwise, [TODO: check details] send the packet using uni-cast bus addressing to each of the potential recepients
	
[TODO: if VIA fields are expected to be used, address this issue] 

NOTE: at terminating device the above steps result in

* check whether the device in the target-list; if found, process the packet by as described in more details while discussing Hmp-From-Santa-Data-Packet below.


At the Root device, forming a From-Santa packet can be organized as follows:

* determine a list of devices to be found and form a Target-Address list
* determine which types of buses have devices to be found and form bus-type-list
* determine a list of retransmitting devices to be used; ultimately, it can be a list of all retransmitters with known routes to, or a subset of this list
* further processing is done as if the Root were a retransmitting device that has received a From-Santa packet with data formed above and that has found itself in the MULTIPLE-RETRANSMITTING-ADDRESSES.

Note: if more than a single device is selected as a target at Root, the payload of the packet must be empty.


Promiscuous Mode Processing
---------------------------

Retransmitting Devices SHOULD, wherever possible, to listen to all the packets in "promiscuous mode". It allows for the following processing:

* if Retransmitting Device hears a packet addressed (at underlying protocol level) to another ("next-hop") Retransmitting Device (which is not Root), and it has a RETRANSMIT-ON-NO-RETRANSMIT flag in Routing Table for the route entry for that Retransmitting Device, and after a TODO timeout it doesn't hear a retransmit (neither full nor "partially correct") by next retransmitting the same packet (TODO define "the same packet"), it MUST try to send a TODO packet to the next-hop Retransmitting Device (in "acknowledged mode") - receiving Device MUST forward the packet to the destination, and send (or attach as a Combined-Packet if the target is Root) a TODO Routing-Error to the Root. If this attempt by our Retransmitting Device doesn't succeed - our Retransmitting Device MUST send a TODO Routing-Error packet (containing the packet as a payload) to the Root.


OPTIONAL-EXTRA-HEADERS
-----------------------

Most of HMP packets have OPTIONAL-EXTRA-HEADERS field. It has a generic structure, but interpretations depend on the packet type. More specifically, OPTIONAL-EXTRA-HEADERS is a sequence of the following items:

* **\| GENERIC-EXTRA-HEADER-FLAGS \|**

  where GENERIC-EXTRA-HEADER-FLAGS is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] indicating the end of OPTIONAL-EXTRA-HEADER list, bits[1..2] equal to 2-bit constant GENERIC_EXTRA_HEADER_FLAGS, and further bits interpreted depending on packet type:

  + bit[3]. MORE-PACKETS-FOLLOW flag.
  + bit[4]. If the packet type is Hmp-To-Santa-Data-Or-Error-Packet or Hmp-Forward-To-Santa-Data-Or-Error-Packet - the bit is IS-ERROR (indicating that PAYLOAD is in fact Routing-Error). If the packet type is Hmp-From-Santa-Data-Packet - it is a TARGET-COLLECT-LAST-HOPS flag. For Hmp-To-Santa-Data-Or-Error-Packet the bit is IS-LOCAL-ECHO flag. For Hmp-Ack-Nack-Packet the bit is IS-NACK flag. For other packet types - RESERVED (MUST be zero)
  + bit[5]. If the packet type is Hmp-From-Santa-Data-Packet, the bit is an EXPLICIT-TIME-SCHEDULING flag. For Hmp-Ack-Nack-Packet - the bit is IS-LOOP-ACK flag. For other packet types - RESERVED (MUST be zero)
  + bit[6]. RESERVED (MUST be zero)
  + bit[7]. If the packet type is Hmp-Unicast-Data-Packet, Hmp-From-Santa-Data-Packet, Hmp-To-Santa-Data-Or-Error-Packet, or Hmp-Forward-To-Santa-Data-Packet - the bit is IS-PROBE flag. For Hmp-Ack-Nack packet - the bit is DELAYS-PRESENT. For other packet types - RESERVED (MUST be zero)
  + bits [8..] - RESERVED (MUST be zeros)

* **\| GENERIC-EXTRA-HEADER-COLLISION-DOMAIN \| COLLISION-DOMAIN-ID-AND-FLAG \| COLLISION-DOMAIN-T0 \| COLLISION-DOMAIN-T1 \| ... \|**

  where GENERIC-EXTRA-HEADER-COLLISION-DOMAIN is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] indicating the end of OPTIONAL-EXTRA-HEADER list, bits[1..2] equal to 2-bit constant GENERIC_EXTRA_HEADER_COLLISION_DOMAIN, and bits [3..] equal to DELAY-UNIT; COLLISION-DOMAIN-ID-AND-FLAG is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=0 indicating the end of collision-domain list, bits[1..] being COLLISION-DOMAIN-ID; COLLISION-DOMAIN-T0 and COLLISION-DOMAIN-T1 are Encoded-Unsigned-Int<max=2> fields specifying respectively beginning and end of the window ("from now") when COLLISION-DOMAIN-ID SHOULD NOT be disturbed.  There can be multiple GENERIC-EXTRA-HEADER-COLLISION-DOMAIN headers in the same packet.

  GENERIC-EXTRA-HEADER-COLLISION-DOMAIN is a special kind of header; on receiving it, each node SHOULD take information within into account, and SHOULD NOT transfer over corresponding COLLISION-DOMAIN-ID within specified time window. In addition, whenever Retransmitting Device retransmits such a packet, it MUST calculate `NEW-COLLISION-DOMAIN-T0 = MAX(0,OLD-COLLISION-DOMAIN-T0 - INCOMING-LINK-DELAY - OUTGOING-LINK-DELAY)` and `NEW-COLLISION-DOMAIN-T1 = MAX(0,OLD-COLLISION-DOMAIN-T1 - INCOMING-LINK-DELAY - OUTGOING-LINK-DELAY + INCOMING-LINK-DELAY-ERROR + OUTGOING-LINK-DELAY-ERROR)` and use `NEW-\*` values in the retransmitted packet; for calculating OLD-COLLISION-DOMAIN-\* parameters DELAY-UNIT field is used, \*-LINK-DELAY parameters together with their DELAY-UNITs are taken from corresponding entries in Routing Table; after doing these calculations, if both NEW-COLLISION-DOMAIN-T0 and NEW-COLLISION-DOMAIN-T1 become =0, this specific extra header SHOULD be dropped (i.e. not sent further).

* **\| UNICAST-EXTRA-HEADER-LOOP-ACK \| LOOP-ACK-ID \|**

  where UNICAST-EXTRA-HEADER-LOOP-ACK is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] indicating the end of OPTIONAL-EXTRA-DATA list, bits[1..2] equal to a 2-bit constant UNICAST_EXTRA_HEADER_LOOP_ACK, and bits[3..] representing NODE-ID of the address where to send the LOOP-ACK, and LOOP-ACK-ID is an Encoded-Unsigned-Int<max=2> field representing ID of the LOOP-ACK to be returned. This extra header MUST NOT be present for packets other than Hmp-Unicast-Data-Packet.

* **\| TOSANTA-EXTRA-HEADER-LAST-INCOMING-HOP \| CONNECTION_QUALITY \|**

  where TOSANTA-EXTRA-HEADER-FLAGS is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] indicating the end of OPTIONAL-EXTRA-HEADER list, bits[1..3] equal to 3-bit constant TOSANTA_EXTRA_HEADER_LAST_INCOMING_HOP, and bits [4..] being node id; and CONNECTION_QUALITY is an Encoded-Unsigned-Int<max=1> bitfield substrate, with bits[0..3] being signal level (with 0 corresponding to the highest and 15 to the lowest signal level) and bits[4..6] being error count (resulting from error correction of the received packet). This extra header MUST NOT be present for packets other than Hmp-To-Santa-Data-Or-Error-Packet. There can be multiple TOSANTA-EXTRA-HEADER-LAST-INCOMING-HOP extra headers within single packet.

*NB: 2-bit extra header type constants MAY overlap as long as applicable types are different.*

HMP Combined-Packet
-------------------

In general, SimpleIoT/HMP passes Combined-Packets over underlying protocol. HMP Combined-Packet consists of one or more HMP Packets as described below; all HMP Packets except for last one in an HMP Combined-Packet, have MORE-PACKETS-FOLLOW flag set (depending on the packet type, this flag is either passed as a part of the first field, or as a part of GENERAL-EXTRA-HEADERS-FLAGS, see details below).

When combining packets, SimpleIoT/HMP MUST take into account both "MTU Hard Limits" and "MTU Soft Limits" of the appropriate SimpleIoT/DLP-\* protocol.

HMP Packets
-----------

Hmp-Unicast-Data-Packet: **\| HMP-UNICAST-DATA-PACKET-FLAGS-AND-TTL \| OPTIONAL-EXTRA-HEADERS \| NEXT-HOP \| LAST-HOP \| Non-Root-Address \| OPTIONAL-PAYLOAD-SIZE \| HEADER-CHECKSUM \| PAYLOAD \| FULL-CHECKSUM \|**

where HMP-UNICAST-DATA-PACKET-FLAGS-AND-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] equal to 0, bit[1] being ACKNOWLEDGED-DELIVERY flag, bit [2] being reserved (must be 0), bit [3] being EXTRA-HEADERS-PRESENT, bit[4] being DIRECTION-FLAG that is set, if a packet follows from the Root, and bits [5..] being TTL; OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT flag is set and is described above; NEXT-HOP is an Encoded-Unsigned-Int<max=2> field containing node ID of the next-hop node (based on info from Routing Table), LAST-HOP is an Encoded-Unsigned-Int<max=2> field containing node ID of currently transmitting node, Non-Root-Address is a target (recipient) address or a source (sender) address depending on DIRECTION-FLAG and is always a device ID of a communication party other than the Root, OPTIONAL-PAYLOAD-SIZE is present only if optional headers are present and MORE-PACKETS-FOLLOW flag is set, and is an Encoded-Unsigned-Int<max=2> field, HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details), PAYLOAD is a payload to be passed to the upper-layer protocol, and FULL-CHECKSUM is a HMP-CHECKSUM of concatenation of the header (without header checksum) and PAYLOAD.

If NEXT-HOP field doesn't match ID of the receiving Device - the packet is ignored.

If a packet is addressed to the Root (DIRECTION-FLAG is not set), it MUST NOT contain VIA fields within.

If IS-PROBE flag is set, then PAYLOAD is treated differently. When destination receives Hmp-Unicast-Data-Packet with IS-PROBE flag set, destination doesn't pass PAYLOAD to upper-layer protocol. Instead, destination parses PAYLOAD as follows: **\| PROBE-TYPE \| OPTIONAL-PROBE-EXTRA-HEADERS \| PROBE-PAYLOAD \|** where PROBE-TYPE is 1-byte bitfield substrate, with bits [0..2] being either PROBE_UNICAST or PROBE_TO_SANTA, bit[3] being PROBE-EXTRA-HEADERS-PRESENT, and bits [4..7] reserved (MUST be zeros); OPTIONAL-PROBE-EXTRA-HEADERS are similar to OPTIONAL-EXTRA-HEADERS, and PROBE-PAYLOAD takes the rest of the PAYLOAD; if PROBE-TYPE==PROBE_UNICAST, then destination Device sends Hmp-Unicast-Data-Packet back to Root, with PAYLOAD copied from PROBE-PAYLOAD, and extra headers formed from PROBE-EXTRA-HEADERS, "as if" this packet is sent in reply to IS-PROBE packet by upper layer, but adding IS-PROBE flag (as a part of GENERIC-EXTRA-FLAGS extra header). If PROBE-TYPE==PROBE_TO_SANTA, destination Device sends a Hmp-To-Santa-Data-Or-Error-Packet, with PAYLOAD copied from PROBE-PAYLOAD, "as if" the packet is sent in reply to IS-PROBE packet by upper layer, but adding IS-PROBE flag (as a part of GENERIC-EXTRA-FLAGS extra header).

Hmp-Unicast-Data-Packet is processed as specified in Uni-Cast Processing section above; if ACKNOWLEDGED-DELIVERY flag is set, packet is sent in 'Acknowledged Uni-Cast' mode. In any case, LAST-HOP field is updated every time the packet is re-sent. Processing at the target node (regardless of node type) consists of passing PAYLOAD to the upper-layer protocol.

If Retransmitting Device receives a "partially correct" Hmp-Unicast-Data-Packet, addressed to itself, and it has NACK-PREV-HOP flag set for the source link within Routing Table, it MUST send a Hmp-Nack-Packet back to the source of packet.

Hmp-From-Santa-Data-Packet: **\| HMP-FROM-SANTA-DATA-PACKET-AND-TTL \| OPTIONAL-EXTRA-HEADERS \| LAST-HOP \| LAST-HOP-BUS-ID \| REQUEST-ID \| OPTIONAL-DELAY-UNIT \| MULTIPLE-RETRANSMITTING-ADDRESSES \| BROADCAST-BUS-TYPE-LIST \| Multiple-Target-Addresses \| OPTIONAL-TARGET-REPLY-DELAY \| OPTIONAL-PAYLOAD-SIZE \| HEADER-CHECKSUM \| PAYLOAD \| FULL-CHECKSUM \|**

where HMP-FROM-SANTA-DATA-PACKET-AND-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=1, bits[1..3] equal to a 3-bit constant HMP_FROM_SANTA_DATA_PACKET, bit [4] being EXTRA-HEADERS-PRESENT, and bits[5..] being TTL; OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT is set, and is described above, LAST-HOP is an Encoded-Unsigned-Int<max=2> representing node id of the last sender, LAST-HOP-BUS-ID is an Encoded-Unsigned-Int<max=2> representing ID of the Bus used by the last sender to send the packet, REQUEST-ID is an Encoded-Unsigned-Int<max=2> field, OPTIONAL-DELAY-UNIT is present only if EXPLICIT-TIME-SCHEDULING flag is present, and is an Encoded-Signed-Int<max=2> field, which specifies units for subsequent DELAY fields (as described below), MULTIPLE-RETRANSMITTING-ADDRESSES is a Multiple-Target-Addresses-With-Extra-Data field described above (with Extra-Data being either empty if EXPLICIT-TIME-SCHEDULING flag is not present, or otherwise Encoded-Unsigned-Int<max=2> DELAY field, using OPTIONAL-DELAY-UNIT field for delay calculations), BROADCAST-BUS-TYPE-LIST is a zero-terminated list of `BUS-TYPE+1` values (enum values for BUS-TYPE TBD), Multiple-Target-Addresses is described above, OPTIONAL-TARGET-REPLY-DELAY has the same type as DELAY fields (and is absent if EXPLICIT-TIME-SCHEDULING flag is not present), and represents delay for the target Device (also using OPTIONAL-DELAY-UNIT field for delay calculations); OPTIONAL-PAYLOAD-SIZE is present only if MORE-PACKETS-FOLLOW flag is set, and is an Encoded-Unsigned-Int<max=2> field; HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details), PAYLOAD is a payload to be passed to the upper-layer protocol, and FULL-CHECKSUM is a HMP-CHECKSUM of concatenation of the header (without header checksum) and PAYLOAD.

Hmp-From-Santa-Data-Packet is a packet sent by Root, which is intended to find one or more destinations specified in Multiple-Target-Addresses that are 'somewhere around', but exact locations are unknown. When Root needs to pass data to a Node for which it has no valid route, or to build a route to one or more Nodes for any other reason, Root sends HMP-FROM-SANTA-DATA-PACKET (or multiple packets), to each of Retransmitting Devices, in hope to find target Device(s) and to pass the packet. It should be noted that if Root intends to pass data to a node within this type of a packet, the packet can be addressed to only a single device, and, therefore, Multiple-Target-Addresses will have only a single address; and if Root intends to find locations to more than a single device at a time, payload must be empty.

Hmp-From-Santa-Data-Packet is processed as specified in "From Santa" packet Processing section above, up to the point where all the buses for all the next hops are found; note that if Multi-Cast processing generates a Routing-Error, it is not transmitted immediately (see below). Starting from that point, Retransmitting Device processes Hmp-From-Santa-Data-Packet proceeds as follows: 

* replaces LAST-HOP field with its own node id
* replaces LAST-HOP-BUS-ID field with its own bus id to be used for packet retransmission
* creates a broadcast-bus-list of its own buses which match BROADCAST-BUS-TYPE-LIST
* for each bus which is on a next-hop-bus list but not on the broadcast-bus-list - continue processing as specified in Multi-Cast Processing section above

  + transmission MUST NOT be made until time specified in DELAY field for current node, passes. If the time in DELAY field (after subtracting `(INCOMING-LINK-DELAY+OUTGOING-LINK-DELAY)` using their respective DELAY-UNITs) has already passed - node MUST introduce a random delay uniformly distributed from 0 to NODE-MAX-RANDOM-DELAY parameter (using NODE-MAX-RANDOM-DELAY-UNIT for calculations).
  + right before sending each modified packet - further modify all DELAY fields within MULTIPLE-RETRANSMITTING-ADDRESSES by subtracting `(INCOMING-LINK-DELAY+OUTGOING-LINK-DELAY)` (using their respective DELAY-UNITs). If resulting value is <0, it is made equal to 0.

* for each bus which is on the broadcast-bus-list - broadcast modified packet over this bus

  + transmission MUST NOT be made until time specified in DELAY field for current node, passes. If the time in DELAY field (after subtracting `(INCOMING-LINK-DELAY+OUTGOING-LINK-DELAY)` using their respective DELAY-UNITs) has already passed - node MUST introduce a random delay uniformly distributed from 0 to NODE-MAX-RANDOM-DELAY parameter (using NODE-MAX-RANDOM-DELAY-UNIT for calculations).
  + right before broadcasting each modified packet - further modify all DELAY (including TARGET-REPLY-DELAY) fields within MULTIPLE-RETRANSMITTING-ADDRESSES by subtracting `(INCOMING-LINK-DELAY+OUTGOING-LINK-DELAY)` (using their respective DELAY-UNITs). If resulting value is <0, it is made equal to 0.

If Retransmitting Device generates Routing-Error, then it MUST be delayed until time of TARGET-REPLY-DELAY + FORWARD-TO-SANTA-DELAY (using corresponding DELAY-UNITs for calculations). If this time has already passed - Routing-Error is transferred with a random delay (from 0 to NODE-MAX-RANDOM-DELAY, using NODE-MAX-RANDOM-DELAY-UNIT for calculations) from now.

On target Device, Hmp-From-Santa-Data-Packet waits until reply payload is ready (which is almost immediately if IS-PROBE is set, including 'discovery' packets, see below), then it is processed as follows:

* if TARGET-DELAY (expressed in DELAY-UNITs) has not passed yet, Device waits until it passes

  + if the incoming packet has TARGET-COLLECT-LAST-HOPS flag set (which is normally set for all the packets which have IS-PROBE flag), then target Device traces all the incoming packets addressed to it and having the same REQUEST-ID and makes a list of extra-last-hops consisting of LAST-HOP and LAST-HOP-BUS-ID headers from all of them
  + when sending Hmp-To-Santa-Data-Or-Error-Packet reply back, target Device adds LAST-INCOMING-HOP extra header for LAST-HOP within incoming packet, *plus* LAST-INCOMING-HOP headers for extra-last-hops (if such list exists, see above)

If IS-PROBE flag is set, then PAYLOAD is treated differently. When destination receives Hmp-From-Santa-Data-Packet with IS-PROBE flag set, destination doesn't pass PAYLOAD to upper-layer protocol. Instead, destination processes the packet in the same way as described for the processing of Hmp-Unicast-Data-Packet with IS-PROBE flag set. A special case of Hmp-From-Santa-Data-Packet with IS-PROBE set is when Target-Address is Root (=0). Such packets (a.k.a. 'discovery' packets) are ignored by Root, but are replied to only by Devices which are not paired yet (i.e. have no node id). All such 'discovery' packets with Target-Address=0 MUST have IS-PROBE flag set.

Hmp-To-Santa-Data-Or-Error-Packet: **\| HMP-TO-SANTA-DATA-OR-ERROR-PACKET-NO-TTL \| OPTIONAL-EXTRA-HEADERS \| SOURCE-ID \| BUS-ID-AT-SOURCE \| REQUEST-ID \| OPTIONAL-PAYLOAD-SIZE \| HEADER-CHECKSUM \| PAYLOAD \| FULL-CHECKSUM \|**

where HMP-TO-SANTA-DATA-OR-ERROR-PACKET-NO-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=1, bits[1..3] equal to a 3-bit constant HMP_TO_SANTA_DATA_OR_ERROR_PACKET, bit[4] being EXTRA-HEADERS-PRESENT, and bits [5..] reserved (MUST be zero); OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT is set, and is described above. Note that Hmp-To-Santa-Data-Or-Error-Packet doesn't contain TTL (as it is never retransmitted 'as is'); SOURCE-ID is an Encoded-Unsigned-Int<max=2> ID of the sender; BUS-ID-AT-SOURCE is an Encoded-Unsigned-Int<max=2> Bus ID used by the sender; REQUEST-ID is an Encoded-Unsigned-Int<max=2> field taken from a Hmp-From-Santa-Data-Packet being answered, or 0, if current packet is initiated by device itself; OPTIONAL-PAYLOAD-SIZE is present only if MORE-PACKETS-FOLLOW flag is set, and is an Encoded-Unsigned-Int<max=2> field; HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details); PAYLOAD is either data or error data depending on IS_ERROR flag; if IS_ERROR flag is set - PAYLOAD format is the same as the body (after OPTIONAL-EXTRA-HEADERS) of Hmp-Routing-Error-Packet; and FULL-CHECKSUM is a HMP-CHECKSUM of concatenation of the header (without header checksum) and PAYLOAD.

If IS-LOCAL-ECHO flag is set, the packet is ignored, except for Retransmitting Devices sending Hmp-Ack-Nack-Packet back to LAST-HOP. To avoid "packet storms", these ACKs MUST be sent using FORWARD-TO-SANTA-DELAY (using FORWARD-TO-SANTA-DELAY-UNIT for calculations). In addition, these ACKs SHOULD contain DELAY-UNIT, DELAY-PASSED, and DELAY-LEFT fields, with DELAY-UNIT being FORWARD-TO-SANTA-DELAY-UNIT, DELAY-PASSED being FORWARD-TO-SANTA-DELAY, and DELAY-LEFT calculated as `MAX-FORWARD-TO-SANTA-DELAY - FORWARD-TO-SANTA-DELAY`. TODO: add RETRANSMITTING-DEVICE-QUALITY?

Hmp-To-Santa-Data-Or-Error-Packet is a packet intended from Device (either Retransmitting or non-Retransmitting) to Root. It is broadcasted by Device in several cases: 

* when the message is marked as Urgent by upper-layer protocol
* when Device needs to report Routing-Error to Root when it has found that Root is not directly accessible.
* when requested to do so via a packet with IS-PROBE flag and PROBE-TYPE==PROBE_TO_SANTA

In any case, if Hmp-To-Santa-Data-Or-Error-Packet is sent in response to a Hmp-From-Santa-Data-Packet flag (regardless of packet being first or not from SimpleIoT/GDP point of view), Device MUST provide TOSANTA-EXTRA-HEADER-LAST-INCOMING-HOP extra header, filling it from LAST-HOP field of the Hmp-From-Santa-Data-Packet.

On receiving Hmp-To-Santa-Data-Or-Error-Packet, Retransmitting Device sends a Hmp-Forward-To-Santa-Data-Or-Error-Packet towards Root, in 'Acknowledged Uni-Cast' mode. To avoid congestion at this point, each Retransmitting Device delays according for FORWARD-TO-SANTA-DELAY (using FORWARD-TO-SANTA-DELAY-UNIT for calculations), where FORWARD-TO-SANTA-DELAY and FORWARD-TO-SANTA-DELAY-UNIT are the values which are locally stored on Retransmitting Device.

Hmp-Forward-To-Santa-Data-Or-Error-Packet: **\| HMP-FORWARD-TO-SANTA-DATA-OR-ERROR-PACKET-AND-TTL \| OPTIONAL-EXTRA-HEADERS \| NEXT-HOP \| FORWARDED-SOURCE-ID \| FORWARDED-BUS-ID-AT-SOURCE \| REQUEST-ID \| OPTIONAL-PAYLOAD-SIZE \| HEADER-CHECKSUM \| PAYLOAD \| FULL-CHECKSUM \|**

where HMP-FORWARD-TO-SANTA-DATA-OR-ERROR-PACKET-AND-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=1, bits[1..3] equal to a 3-bit constant HMP_FORWARD_TO_SANTA_DATA_OR_ERROR_PACKET, bit [4] being EXTRA-HEADERS-PRESENT, and bits [5..] being TTL; OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT is set, and is described above; NEXT-HOP is an Encoded-Unsigned-Int<max=2> field containing node ID of the next-hop node (based on info from Routing Table); FORWARDED-SOURCE-ID is an Encoded-Unsigned-Int<max=2> value of SOURCE-ID from the original To-Santa packet; FORWARDED-BUS-ID-AT-SOURCE is an Encoded-Unsigned-Int<max=2> value of BUS-ID-AT-SOURCE from the original To-Santa packet; REQUEST-ID is an Encoded-Unsigned-Int<max=2> field; OPTIONAL-PAYLOAD-SIZE is present only if MORE-PACKETS-FOLLOW flag is set, and is an Encoded-Unsigned-Int<max=2> field; HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details); PAYLOAD is data being forwarded (copied from PAYLOAD of Hmp-To-Santa-Data-Or-Error-Packet); and FULL-CHECKSUM is a HMP-CHECKSUM of concatenation of the header (without header checksum) and PAYLOAD.

If NEXT-HOP field doesn't match ID of the receiving Device - the packet is ignored.

Hmp-Forward-To-Santa-Data-Or-Error-Packet is sent by Retransmitting Device when it receives Hmp-To-Santa-Data-Or-Error-Packet (with TTL=MAX_TTL-1 to account for original Hmp-To-Santa-Data-Or-Error-Packet). On receiving Hmp-Forward-To-Santa-Data-Or-Error-Packet by a Retransmitting Device, it is  processed as described in Uni-Cast processing section above (with implicit Target-Address being Root), and is always sent in 'Acknowledged Uni-Cast' mode.

Hmp-Routing-Error-Packet: **\| HMP-ROUTING-ERROR-PACKET-AND-TTL \| OPTIONAL-EXTRA-HEADERS \| LAST-HOP \| ERROR-CODE \| HEADER-CHECKSUM \| PAYLOAD \| FULL-CHECKSUM \|**

where HMP-ROUTING-ERROR-PACKET-AND-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=1, bits[1..3] equal to a 3-bit constant HMP_ROUTING_ERROR_PACKET, bit [4] being EXTRA-HEADERS-PRESENT, and bits [5..] being TTL; OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT is set, and is described above; LAST-HOP is an Encoded-Unsigned-Int<max=2> representing node id of the last sender; ERROR-CODE is an Encoded-Unsigned-Int<max=1> field, HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details), PAYLOAD is TODO, and FULL-CHECKSUM is a full-packet HMP-CHECKSUM.

On receiving Hmp-Routing-Error-Packet, it is processed as described in Uni-Cast processing section above (with implicit Target-Address being Root), and is always sent in 'Acknowledged Uni-Cast' mode.

Hmp-Ack-Nack-Packet: **\| HMP-ACK-NACK-AND-TTL \| OPTIONAL-EXTRA-HEADERS \| LAST-HOP \| Target-Address \| NUMBER-OF-ERRORS \| ACK-CHESKSUM \| HEADER-CHECKSUM \| OPTIONAL-DELAY-UNIT \| OPTIONAL-DELAY-PASSED \| OPTIONAL-DELAY-LEFT \| FULL-CHECKSUM \|**

where HMP-ACK-NACK-AND-TTL is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0]=1, bits[1..3] equal to a 3-bit constant HMP_ACK_NACK_PACKET, bit [4] being EXTRA-HEADERS-PRESENT, and bits [5..] being TTL; OPTIONAL-EXTRA-HEADERS is present only if EXTRA-HEADERS-PRESENT flag is set, LAST-HOP is an id of the transmitting node, Target-Address is described above, NUMBER-OF-ERRORS is an Encoded-Unsigned-Int<max=2> field, which contains number of bit-errors observed at PHY level for the packet being acknowledged, ACK-CHECKSUM is copied from FULL-CHECKSUM of the packet being acknowledged (with an exception for NACK generated due to "partially correct" packet, see below), and HEADER-CHECKSUM is a header HMP-CHECKSUM (see HMP-CHECKSUM section for details); OPTIONAL-DELAY-UNIT, OPTIONAL-DELAY-PASSED, and OPTIONAL-DELAY-LEFT fields are all Encoded-Unsigned-Int<max=2> fields, all present only if DELAYS-PRESENT flag is set (which is set only in response to packets with IS-LOCAL-ECHO flag set, see above); and FULL-CHECKSUM is a HMP-CHECKSUM of concatenation of the header (without header checksum) and the remaining part of the packet.

NUMBER-OF-ERRORS field allows to provide feedback about connection quality to sender by receiver; it is a normalized number of bit errors which have been error-corrected when the packet being acknowledged, was received by receiver. If error correction is not employed, this field SHOULD be zero. This information SHOULD be used by sending-side PHY level to optimize power consumption.

Hmp-Ack-Nack-Packet with IS-LOOP-ACK flag is generated either by destination, or by the node which has found that the next hop already has NEXT-HOP-ACKS flag (see details in 'Acknowledged Uni-Cast' section above); generating node always specifies itself as a target. Hmp-Ack-Nack-Packet with IS-LOOP-ACK flag MUST NOT have IS-NACK flag.

If Hmp-Ack-Nack-Packet has IS-LOOP-ACK flag, it is processed as specified in 'Uni-cast processing' section above; Hmp-Loop-Ack packet is never sent using 'Acknowledged uni-cast' delivery. Processing at the target node (regardless of node type) consists of passing PAYLOAD to the upper-layer protocol.

Hmp-Ack-Nack-Packet without IS-LOOP-ACK flag and without IS-NACK flag, is generated as a response to an incoming Hmp-Unicast-Data-Packet with ACKNOWLEDGED-DELIVERY flag, or in response to a packet with IS-LOCAL-ECHO flag (TODO: anything else?). It is not retransmitted, but taken as an acknowledgement that the packet has been received. 

In addition, Hmp-Ack-Nack-Packet without IS-LOOP-ACK flag and without IS-NACK flag, MAY be generated by receiver in an "unsolicited" manner, i.e. even if ACK has not been requested, to indicate that received packet has number of errors which is considered to be "too high" for the underlying PHY level. Such an ACK packet (as well as any other ACK packet with high NUMBER-OF-ERRORS) SHOULD lead to adjustments on sending side (for example, it MAY lead to increase in transmission power). Another case for "unsolicited" ACK is for Retransmission Device, when NUMBER-OF-ERRORS becomes "too low" after being substantially higher, to indicate that the other side is allowed to lower transmission power. In any case, whenever Retransmission Device sends an "unsolicited" ACK to non-transmitting Device , it SHOULD make sure (from upper-layer protocols) that receiving non-transmitting Device is expected to have it's transceiver on.

Hmp-Ack-Nack-Packet without IS-LOOP-ACK flag and with IS-NACK flag, is generated as a response to a "partially correct" packet (regardless of type and ACKNOWLEDGED-DELIVERY flag); in this case, it's ACK-CHECKSUM represents only HEADER-CHECKSUM of the original packet. Such Hmp-Ack-Nack-Packet is not retransmitted itself, but is taken as an indication to perform quick retransmit of the last packet sent.

Type of HMP packet
^^^^^^^^^^^^^^^^^^^

As described above, type of HMP packet is always defined by bits [0..3] of the first field (which is always Encoded-Unsigned-Int<max=2> bitfield substrate):

+-------------------------------------+--------------------------------------------+--------------------------------------------+
| bit [0]                             | bits[1..3]                                 | HMP packet type                            |
+=====================================+============================================+============================================+
| 0                                   | ANY (used for other purposes)              | Hmp-Unicast-Data-Packet                    |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | HMP_FROM_SANTA_DATA_PACKET                 | Hmp-From-Santa-Data-Packet                 |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | HMP_TO_SANTA_DATA_OR_ERROR_PACKET          | Hmp-To-Santa-Data-Packet                   |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | HMP_FORWARD_TO_SANTA_DATA_OR_ERROR_PACKET  | Hmp-Forward-To-Santa-Data-Or-Error-Packet  |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | HMP_ROUTING_ERROR_PACKET                   | Hmp-Routing-Error-Packet                   |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | HMP_ACK_NACK_PACKET                        | Hmp-Ack-Nack-Packet                        |
+-------------------------------------+--------------------------------------------+--------------------------------------------+
| 1                                   | 3 more values                              | RESERVED                                   |
+-------------------------------------+--------------------------------------------+--------------------------------------------+

Packet Urgency
--------------

From SimpleIoT/HMP point of view, all upper-layer-protocol packets can have one of three urgency levels. If the packet has urgency URGENCY_LAZY, it is first sent as a Hmp-Unicast-Data-Packet without ACKNOWLEDGED-DELIVERY flag (as described above, in case of retries it will be resent with ACKNOWLEDGED-DELIVERY). If the packet has urgency URGENCY_QUITE_URGENT, it is first sent as a Hmp-Unicast-Data-Packet with ACKNOWLEDGED-DELIVERY flag (as described above, in case of retries it will be resent as a Hmp-\*-Santa-\* packet). If the packet has urgency URGENCY_TRIPLE_GALOP, 
then it is first sent as a Hmp-From-Santa-Data-Packet or Hmp-To-Santa-Data-Packet (depending on source being Root or Device). 

Handling of TERMINAL-ADVERTISING Underlying Protocols
-----------------------------------------------------

Some of underlying SimpleIoT/DLP-\* protocols MAY be designated as TERMINAL-ADVERTISING ones. For these protocols, some of the handling is different from described above. In particular:

* Hmp-From-Santa packets are never sent with BUS-TYPE being a TERMINAL-ADVERTISING bus. 

  + If, according to the normal HMP logic described above, a need arises to send Hmp-From-Santa packet with such a BUS-TYPE, this BUS-TYPE is simply skipped.
  + If, as a result of such filtering, BUS-TYPE-LIST of Hmp-From-Santa packet becomes empty, Hmp-From-Santa packet is not sent at all

* Whenever TERMINAL-ADVERTISING Device has its transmitter turned on, but it has no connection (as defined in respective SimpleIoT/DLP-\* document) to the next hop, it starts to "advertise" itself (as defined in respective SimpleIoT/DLP-\* document), using an Hmp-To-Santa packet as a payload. This Hmp-To-Santa packet MAY be a packet-which-needs-to-be-delivered-to-Root, or MAY be an Hmp-To-Santa packet with an empty payload (TODO: define). 

  + All Retransmitting Devices which hear this "advertised" Hmp-To-Santa packet, process it as a normal Hmp-To-Santa packet
  + When Hmp-Forward-To-Santa packets reach Root:
    
    - Root chooses "the best" route, and assumes that all the inter-hops connections are symmetrical (i.e. path from A to B always implies path from B to A).
    - Root updates Routing Tables along the chosen route (the same way as for non-TERMINAL-ADVERTISING Devices)

      - Retransmitting Device which is adjacent to the TERMINAL-ADVERTISING Device which has advertised, established connection with the Device (as defined in respective SimpleIoT/DLP-\* document). If connection cannot be established, Retransmitting Device sends a TODO Hmp-Routing-Error-Packet to Root.

      - If, at any point, connection is broken (as defined in respective SimpleIoT/DLP-\* document), Retransmitting Device sends a TODO Hmp-Routing-Error-Packet to Root.

PHY quality measurement over SimpleIoT/HMP
------------------------------------------

Certain SimpleIoT/DLP-\* protocols need to measure connection quality. This can be made using the following procedure:

* Device sends Hmp-To-Santa packet with IS-LOCAL-ECHO flag
* Device waits for any Hmp-Ack-Nack packet, validly acknowledging receipt of IS-LOCAL-ECHO packet, OR for 100 milliseconds, whichever comes first
* If a valid Hmp-Ack-Nack packet is received - Device waits only for DELAY-LEFT specified in the packet from the moment of receiving the packet (more strictly: if multiple packets are received, it is maximum of the DELAY-LEFT-received-since-receiving-each-packet + 10ms (safety margin)).
* While waiting, all the valid Hmp-Ack-Nack packets are accounted for (to be used as described in respective SimpleIoT/DLP-\* document)
* when wait expires, Device repeats the whole process above; 5 repetitions are usually made to gather required statistics.

This "quality measurement" procedure MAY be performed ONLY if respective SimpleIoT/DLP-\* document specifies using it, and ONLY under circumstances specified there.

Device Discovery and Pairing over SimpleIoT/HMP
-----------------------------------------------

Whenever Device is in PRE-PAIRING state (see :ref:`siot_pairing` for details on the PRE-PAIRING state), it scans all available channels; if channel is "eligible" (as defined in an appropriate SimpleIoT/DLP-\* document), the following basic exchange occurs:

* Device (after, maybe, performing certain preliminary actions on the channel, as defined in an appropriate SimpleIoT/DLP-\* document) sends Pairing-Ready-Pseudo-Response (described in :ref:`siot_pairing` document), as a SimpleIoT/CCP packet, addressed to Root. When SimpleIoT/CCP packet reaches SimpleIoT/HMP level (still on Device side), SimpleIoT/HMP doesn't have a route to Root, so it sends it as a Hmp-To-Santa packet. 
* In response, Root will send a Pairing-Pre-Request (as it has no route to Device, it will be sent as a  From-Santa HMP packet)
* Device will reply with Pairing-Pre-Response (which will be sent a To-Santa HMP packet, containing DEVICE-INTRABUS-ID)
* *Up to this point in exchange, all the packets from Root to Device at SimpleIoT/HMP level, including optional and not mentioned above Entropy Gathering packets, are always sent as From-Santa packets with Target-Address being ROOT, i.e. broadcast packets. Packets from Device to Root are sent as To-Santa packets.*
* *From this point onwards, all the packets from Root to Device at SimpleIoT/HMP level are always addressed to specific Device, using non-paired addressing. Packets from Device to Root are still sent as To-Santa packets.*
* Root will proceed with Pairing procedure as described in :ref:`siot_pairing` document, still using HMP From-Santa/To-Santa packets, but from now on From-Santa packets are addressed to specific Device using "non-paired addressing"
* As soon as Device pairing is completed (and Root sets NODE-ID for the Device), Root SHOULD:

  + calculate optimal route to the Device
  + change Routing Tables for all the Retransmitting Devices alongside the optimal route (for example, using CCP_PHY_AND_ROUTING_DATA packets as described above)
  + as soon as confirmations from all the Retransmitting Devices about route updates are obtained, Root SHOULD start using Device's "paired addressing" for all the communications onwards with the Device.
  + change Routing Table on the Device, indicating optimal route to the Root. From this point on, Device will start using usual Unicast packets when communicating with Root (unless there are reasons to use other HMP packets, for example, on multiple retransmits or for packets marked URGENT).

TODO: merge of To-Santa into Unicast (with NEXT-HOP being -1)?
TODO: Hmp-Retransmit (to next-hop Retransmitting Device on RETRANSMIT-ON-NO-RETRANSMIT)
TODO: define handling for all "partially correct" packets
TODO: what exactly is "header" for the purposes of "partially correct" packets? Is "sub-header" worth the trouble?
TODO: NACK-PREV-HOP into Routing Table Links; RETRANSMIT-ON-NO-RETRANSMIT into RT Routes
TODO: ?move FORWARD-TO-SANTA-\* to links (target ones) too (and specify that it is per-link wherever it is used)
TODO: procedure for calibration of LINK-DELAYs?
TODO: optional explicit loop begin (alongside VIA?)

