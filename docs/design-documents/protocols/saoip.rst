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

.. _saoip:

SmartAnthill-over-IP Protocol (SAoIP) and SmartAnthill Router
=============================================================

:Version:   v0.2.2

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SAoIP is a part of SmartAnthill 2.0 protocol stack. It belongs to Layer 4 of OSI/ISO Network Model, and is responsible for transferring SAoIP payload (usually SASP packets) between SmartAnthill Client (normally implemented by SmartAnthill Core) and SmartAnthill Device or SmartAnthill Router.

Within SmartAnthill protocol stack, SAoIP is located right below SASP. SAoIP is used both to communicate to *SmartAnthill IP-Enabled Devices*, and to *SmartAnthill Simple Devices*. However, *SmartAnthill Simple Devices* don't implement IP stack (neither they implement SAoIP). For *SmartAnthill Simple Devices* SAoIP is processed by respective *SmartAnthill Router*, which takes away SAoIP headers (obtaining SAoIP payload, which is normally bare SASP packets), and then wraps this SAoIP payload into respective SADLP-\* packets, to be passed to respective *SmartAnthill Simple Device*. 

Despite this difference in handling of SAoIP between *SmartAnthill IP-Enabled Devices* and *SmartAnthill Simple Devices*, from the point of view of SmartAnthill Client these SmartAnthill Devices are completely indistinguishable, and both SHOULD be addressed in the very same manner over SAoIP.

.. contents::


SAoIP Flavours
--------------

SAoIP may be implemented in several flavours: SAoUDP, SAoTCP, and SAoTLSoTCP. 

SmartAnthill Clients and SmartAnthill Routers SHOULD implement all SAoIP flavours. *SmartAnthill IP-Enabled Devices* MAY implement only one flavour. In particular, implementing only SAoUDP may allow to avoid implementing (or running) resource-expensive TCP stack, allowing *SmartAnthill IP-Enabled Device* to implement only IP stack (w/o TCP stack) plus SAoUDP (w/o SAoTCP or SAoTLSoTCP).

SAoIP Requirements
------------------

Generally, SAoIP is a very simple wrapper around SAoIP payload (which are normally SASP packets). As guaranteed delivery is normally handled by SAGDP, no guarantees are required (and neither are provided) by SAoIP in general. Even when certain SAoIP flavour (such as SAoTCP) provides certain delivery guarantees, SAoIP application layer (normally SASP+SAGDP+SACCP) MUST NOT rely on delivery guarantees provided by specific SAoIP flavour.

SAoIP Addressing
----------------

As with the rest of SmartAnthill Protocol Stack, each SmartAnthill Device in SAoIP is identified by it's own unique address: triplet (IPv6-address:SAoIP-flavour:port-number). 

SAoIP Aggregation and Destination-\* fields
-------------------------------------------

SAoIP Aggregation is an OPTIONAL feature of SAoIP which allows to reduce number of IP addresses/ports necessary for SmartAnthill Router to keep open. If SAoIP aggregation is used, then a triplet of special fields (Flavour,Destination-IP - which is always a 16-byte IPv6 field,Destination-Port), is used to distinguish which of the SmartAnthill Devices the request is addressed to. To use SAoIP Aggregation, SmartAnthill Client should have a mapping between target-device-IP and address-of-SmartAnthill-Router-which-is-ready-to-process-this-IP-device; let's name this mapping *router-address(target-IP-address)*. As soon as this mapping is known, SmartAnthill Client should send requests which are intended to *target-device-IP*, to *router-address(target-IP-address)*, while setting *Destination-IP* field within respective SAoIP header to *target-IP-address*.

If SAoIP Aggregation is not in use, then Destination-IP field MUST NOT be present in the data transferred (for example, for SAoUDP, SAOUDP_HEADER_AGGREGATE MUST NOT be present). If *SmartAnthill Router* receives a packet on a non-aggregated port, and the packet has Destination-IP field, it SHOULD drop this packet as an invalid one (possibly with logging).


SAoIP SCRAMBLING
----------------

SCRAMBLING is an optional feature of SAoIP. SAoIP SHOULD use SCRAMBLING whenever SAoIP goes over non-secure connection; while not using SCRAMBLING is not a significant security risk, but might reveal some information about packet destination and/or might simplify certain DoS attacks. For the purpose, any connection SHOULD be considered as exposed (and therefore SCRAMBLING procedure SHOULD be used) unless proven secure; in particular, all connections which go over Wi-Fi or over the Internet, SHOULD be considered as exposed.

SAoIP uses SCRAMBLING procedure as described in :ref:`sascrambling` document. 

SCRAMBLING requires that both parties share the same symmetric key (which normally consists out of two Speck-96 keys, as described in :ref:`sascrambling` document). **This symmetric key MUST be completely independent and separate from any other keys, in particular, from SASP keys**. 

To comply with requirements of SCRAMBLING procedure (as described in :ref:`sascrambling` document), SAoIP needs to calculate offset of the *unique-block* within SAoIP packet; for SAoIP, it always equals to *unique-block-offset* returned by SASP, and adjusted by position of SASP packet within SAoIP packet.

SCRAMBLING being optional
^^^^^^^^^^^^^^^^^^^^^^^^^

In some cases (for example, if all the communications is within Intranet without being passed through wireless links, or performed over TLS), SAoIP MAY omit SCRAMBLING procedure. In fact, if there is no information about SCRAMBLING key for the packet sender, both SmartAnthill Router and SmartAnthill IP-Enabled Device SHOULD try to interpret the packet as the one without SCRAMBLING applied. 

Formally, within SmartAnthill Protocol Stack omitting SCRAMBLING doesn't affect any security guarantees (as such guarantees are provided by SASP, which is not optional). However, as SCRAMBLING provides some benefits at a very low cost, by default SCRAMBLING procedure SHOULD be applied to all communications which are potentially exposed to the attacker.

SAoIP Pre-Packet
----------------

All SAoIP flavours are using so-called "SAoIP Pre-Packet". SAoIP pre-packet is not sent directly over UDP or TCP, but is used as a building block for other packets as described below. SAoIP pre-packet has the following format: 

**\| Headers \| SAoIP-Payload \|**

where Headers are optional SAoIP headers; the idea of SAoIP Headers is remotely similar to that of IP optional headers. If receiver gets a message with some of Headers which are not known to it, it MUST ignore the header and SHOULD sent a TODO packet (vaguely similar to ICMP 'Parameter Problem' message) back to the sender. 

The last Header is always a SAOIP_HEADER_LAST_HEADER header. Therefore, if there are no extensions, SAoIP pre-packet looks as **\| SAOIP_HEADER_LAST_HEADER \| SAoIP-Payload \|  \|**.

All Headers (except for LAST_HEADER, which is described below) have the following format: 

**\| Header-Type \| Data-Length \| Data \|**

where Header-Type is an Encoded-Unsigned-Int<max=2> field, Data-Length is also an Encoded-Unsigned-Int<max=2> field, and Data is a variable-length field which has Data-Length size.

Currently supported extensions are:

**\| SAOIP_HEADER_AGGREGATE_REQUEST \| Data-Length \| Destination-Flavour \| Destination-IPv6 \| Destination-Port \| Reply-ID \|**

where Destination-Flavour is a 1-byte field, Destination-IPv6 is a 16-byte field containing IPv6 address, Destination-Port is a 2-byte field (using SmartAnthill Endianness), and Reply-ID is an Encoded-Unsigned-Int<max=10> field. The meaning and handling of Destination-IPv6, Destination-Flavour, and Destination-Port fields is described in "SAoIP Aggregation and Destination-\* fields" section above. 

SAOIP_HEADER_AGGREGATE_REQUEST is used only for packets which travel from SmartAnthill Client to SmartAnthill Router. Reply-ID is a field which is returned in the reply (or replies) to this request. 

**\| SAOIP_HEADER_AGGREGATE_REPLY \| Data-Length \| Reply-ID \|**

where Reply-ID is an Encoded-Unsigned-Int<max=10> field

SAOIP_HEADER_AGGREGATE_REPLY is used only for packets which travel from SmartAnthill Router to SmartAnthill Client. Reply-ID is a field which was sent in the last SAOIP_HEADER_AGGREGATE_REQUEST from the SmartAnthill Client. 

**\| SAOIP_HEADER_LAST_HEADER \|**

SAOIP_HEADER_LAST_HEADER is always the last header in the header list. Indicates that immediately after this header, SAoIP-Payload field is located. Note that LAST_HEADER doesn't have a 'Data-Length' field.

SAoUDP
------

SAoUDP is one of SAoIP flavours, which operates over UDP.

SAoUDP packet
^^^^^^^^^^^^^

To form SAoUDP packet, SAoUDP handler first creates a SAoIP pre-packet (which is described above). Then, this SAoIP pre-packet is SCRAMBLED (using SCRAMBLING procedure) to obtain SAoUDP packet. Then, SAoUDP packet is sent as a UDP datagram.

SAoUDP and UDP
^^^^^^^^^^^^^^

SAoUDP packet uses UDP as an underlying transport; as such, it also (implicitly) contains standard 8-byte UDP headers as described in RFC 768. SAoUDP only uses unicast UDP. 

As we see it, SAoUDP (when used with the rest of the SmartAnthill Protocol Stack) is compliant with RFC5405 ("Unicast UDP Usage Guidelines for Application Designers"), and is therefore formally suitable for use in public Internet. However, for practical reasons (because of UDP-hostile firewalls, because of not-properly-implemented or unsupported UDP NAT on many routers, and because of potential for DoS attacks unless SCRAMBLING is used), use of SAoUDP on public Internet is discouraged. Use of SAoUDP in LANs or Intranets is perfectly fine. 

SAoUDP Packet Sizes and Payloads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To comply with RFC 5405, SAoUDP SHOULD restrict maximum IP packet to the size of 576 bytes [1]_. Taking into account IP and UDP headers, it means that SAoUDP packet SHOULD be restricted to `576-60-8=508` bytes, and taking into account maximum size of supported SAoUDP headers, SAoIP-Payload for SAoUDP SHOULD be restricted to 508-TODO=TODO. This is a value which SHOULD be used for calculations of the maximum *Client_Side_SACCP_payload* as used in :ref:`saprotostack` document. For example, if SAoUDP payload size is typical TODO bytes (as calculated above), then corresponding maximum SASP payload is TODO+7bits, maximum SAGDP payload is TODO bytes, and maximum SACCP payload (and therefore *Client_Side_SACCP_payload*) is also TODO bytes.

.. [1] Strictly speaking, RFC 5405 says that MTU should be less than `min(576,first-hop-MTU)`; if first-hop-MTU on an interface which SmartAnthill Client uses, is less than 576, maximum SACCP payload SHOULD be recalculated accordingly; note that due to the block nature of SASP, dependency between SAoUDP payload and SACCP payload in not exactly linear and needs to be re-calculated carefully; however, MTU being less than 576 is very unusual these days.

SAoTCP
------

SAoTCP is one of SAoIP flavours, which operates over TCP. Normally, SmartAnthill Client acts as a TCP client, and SmartAnthill Device (or SmartAnthill Router) acts as a TCP server (i.e. listens on a TCP socket).

SAoTCP "Streamed SCRAMBLING"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As SAoTCP is a stream, it uses "Streamed SCRAMBLING" procedure as described in :ref:`sascrambling` document, sending *Streamed-SCRAMBLING pseudo-packets* formed by "Streamed SCRAMBLING", over TCP.

SAoTCP stream decoding
''''''''''''''''''''''

To ensure proper error recovery, receiving side of SAoTCP implementation MUST forcibly break a TCP connection as soon as any of the de-SCRAMBLING operations for packets received over this TCP connection fail. This forced break of TCP connection SHOULD be implemented with RST sent back and without wait (see lingering options of TCP socket for implementation details). After such a forced-break, SmartAnthill Client SHOULD re-establish a TCP connection.

SAoTLSoTCP
----------

SAoTLSoTCP is one of SAoIP flavours, which operates over TLS which runs over TCP. Normally, SmartAnthill Client acts as a TCP client, and SmartAnthill Device (or SmartAnthill Router) acts as a TCP server (i.e. listens on a TCP socket). SAoTLSoTCP operates exactly as SAoTCP, with the only difference being that SAoTLSoTCP uses "TLS over TCP" as it's underlying protocol. 

TODO: TLS specifics (which TLS versions are allowed/recommended)

TODO: dual connections (take-over?) - for SAoTCP

TODO: QoS (retransmit times?) - for all SAoIP


SmartAnthill Router
-------------------

SmartAnthill Router is responsible for handling incoming SAoIP packets (for example, SAoUDP packets) and translating them into SADLP-\* packets. 

To do this, SmartAnthill Router keeps the following records in SmartAnthill Database (SA DB) table DEVICE_MAPPINGS): 

**\| Device-Key-ID \| IPv6 \| SAoIP-Flavour \| port \| SCRAMBLING-Key \| Bus ID \| Intra-Bus ID \| Recrypt-External-Key \| Recrypt-Internal-Key \|**

In addition, there is another SA DB table KEY_MAPPINGS:

**\| Device-Key-ID \| external-SASP-key-ID \| internal-SASP-key-ID \|**

When an incoming SAoIP packet comes in (to a normal, non-aggregated port, from a certain socket), SmartAnthill Router: 

* finds out an address of the receiving socket: (Flavour,IPv6,port). If socket listens on IPv4, IPv4 is first translated into IPv6 using "Stateless IP/ICMP Translation" (SIIT).
* finds out a 'from' address of the packet: (Flavour,IPv6,port); normally, it is taken from the incoming packet of SAoIP underlying protocol (for example, from UDP packet itself). If TCP or UDP operates over IPv4, then IPv4 is first translated into IPv6 using "Stateless IP/ICMP Translation" (SIIT).
* checks if any filtering rules apply to the 'from' address (TODO: define filtering rules a-la IPTables)
* finds a record in DEVICE_MAPPINGS table, based on (IPv6,Flavour,port); from this record, obtains Device-Key-ID, SCRAMBLING-Key, and (Bus-ID,Intra-Bus-ID) pair
* if SCRAMBLING-Key is not NULL, DESCRAMBLES incoming packet (using SCRAMBLING-Key)
* at this point we have a plain (not scrambled) SAoIP packet
* parses SAoIP packet to get SASP packet, and gets key-ID from SASP packet (it can be extracted without decrypting SASP packet); for SmartAnthill Router, this is external-SASP-key-ID.
* finds a row in KEY_MAPPINGS based on Device-Key-ID and external-SASP-key-ID; gets internal-SASP-key-ID. TODO: what to do if record is not found
* if DEVICE_MAPPINGS record found above, contains "re-crypt" information (which is a pair of Recrypt-External-Key and Recrypt-Internal-Key), SmartAnthill Router decrypts SASP packet within SAoIP-Payload (using Recrypt-External-Key) and encrypts it again (using Recrypt-Internal-Key)
* changes ('hacks') SASP packet to use internal-SASP-key-ID instead of external-SASP-key-ID; this can be done without decrypting SASP packet
* forms a SADLP-\* packet (depending on the bus in use) as described in respective document, using SASP 'hacked' packet as a payload
* sends SADLP-\* packet to (Bus-ID, Intra-Bus-ID)
* makes a record in a special SA DB table KEY_LEASES, specifying that Device-Key-ID (from DEVICE_MAPPINGS record) corresponds to a reply-to address (i.e. where to send replies). Reply-to address is the same as 'from' address of the incoming packet. If there is already a record in KEY_LEASES with the same Device-Key-ID, it is replaced with a new one (and a log record is made about lease being taken over). 

When an incoming packet from SADLP-\* comes in (from certain Bus-ID and Intra-Bus-ID), SmartAnthill Router:

* processes SADLP-\* incoming packet to obtain SAoIP packet, as described in respective document
* parses SAoIP packet to get SASP packet, and gets key-ID out of it (this can be done without decrypting SASP packet); for SmartAnthill Router, this is internal-SASP-key-ID
* finds a row in DEVICE_MAPPINGS table, based on (Bus ID, Intra-Bus ID), and obtains Device-Key-ID and SCRAMBLING-Key TODO: what to do if not found
* finds a row in KEY_MAPPINGS table, based on (Device-Key-ID, internal-SASP-key-ID), and obtains external-SASP-key-ID TODO: what to do if not found
* finds a row in SA DB table KEY_LEASES, based on Device-Key-ID, and obtains reply-to address TODO: what to do if not found
* changes ('hacks') SASP packet to use external-SASP-key-ID instead of internal-SASP-key-ID; this can be done without decrypting SASP packet
* if DEVICE_MAPPINGS record found above, contains "re-crypt" information, SmartAnthill Router decrypts SASP packet within SAoIP-Payload (using Recrypt-Internal-Key) and encrypts it again (using Recrypt-External-Key)
* forms a SAoIP packet, using reply-to address, and 'hacked' SASP packet as a payload
* if SCRAMBLING-Key is not NULL, SCRAMBLES packet, using SCRAMBLING-Key
* sends packet to reply-to address

Handling SmartAnthill Aggregation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If an incoming SAoIP packet is a valid SmartAnthill Aggregation request, then SmartAnthill Router additionally performs the following steps:

* fills in additional field Aggregation-Reply-ID in KEY_LEASES table (from Reply-ID field in the packet)

If for an incoming SADLP-\* packet an Aggregation-Reply-ID in KEY_LEASES record is not NULL:

* sends a reply as a SmartAnthill Aggregation reply, with Reply-ID set to Aggregation-Reply-ID from KEY_LEASES record


TODO: buffering if there is no TCP connection to reply to

