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

SmartAnthill-over-IP Protocol (SAoIP)
=====================================

:Version:   v0.1.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SAoIP is a part of SmartAnthill 2.0 protocol stack. It belongs to Layer 4 of OSI/ISO Network Model, and is responsible for transferring SAoIP payload (usually SASP packets) between SmartAnthill Client (normally implemented by SmartAnthill Core) and SmartAnthill Device or SmartAnthill Router.

Within SmartAnthill protocol stack, SAoIP is located right below SASP. SAoIP is used both to communicate to *SmartAnthill IP-Enabled Devices*, and to *SmartAnthill Simple Devices*. However, *SmartAnthill Simple Devices* don't implement IP stack (neither they implement SAoIP). For *SmartAnthill Simple Devices* SAoIP is processed by respective *SmartAnthill Router*, which takes away SAoIP headers (obtaining SAoIP payload, which is normally bare SASP packets), and then wraps this SAoIP payload into respective SADLP-* packets, to be passed to respective *SmartAnthill Simple Device*. 

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

SAoIP Aggregation and Destination-IPv6 field
--------------------------------------------

SAoIP Aggregation is an OPTIONAL feature of SAoIP which allows to reduce number of IP addresses/ports necessary for SmartAnthill Router to keep open. If SAoIP aggregation is used, then a special field Destination-IP (which is always a 16-byte IPv6 field), is used to distinguish which of the SmartAnthill Devices the request is addressed to. To use SAoIP Aggregation, SmartAnthill Client should have a mapping between target-device-IP and address-of-SmartAnthill-Router-which-is-ready-to-process-this-IP-device; let's name this mapping *router-address(target-IP-address)*. As soon as this mapping is known, SmartAnthill Client should send requests which are intended to *target-device-IP*, to *router-address(target-IP-address)*, while setting *Destination-IP* field within respective SAoIP header to *target-IP-address*.

If SAoIP Aggregation is not in use, then Destination-IP field MUST NOT be present in the data transferred (for example, for SAoUDP, SAOUDP_HEADER_AGGREGATE MUST NOT be present). If *SmartAnthill Router* receives a packet on a non-aggregated port, and the packet has Destination-IP field, it SHOULD drop this packet as an invalid one.

Reverse Parsing and Reverse-Encoded-Size
----------------------------------------

To comply with requirements of SAScP (TODO: SAScp), headers in SAoIP are usually located in the end of the packet. As a result, parsing should be performed starting from the end of the packet. To facilitate such a 'reverse parsing', 'Reverse-Encoded-Size' encoding is used; Reverse-Encoded-Size<max=n> encoding is identical to Encoded-Size<max=2> encoding as defined in :ref:`saprotostack` document, except that all the bytes are written (and parsed) in the reverse order.

SAoUDP
------

SAoUDP is one of SAoIP flavours. SAoUDP packet looks as follows:

**\| SAoIP-Payload \| Headers \|**


where Headers are optional headers for the SAoUDP; the idea of SAoUDP Headers is remotely similar to that of IP optional headers. If receiver gets a message with some of Headers which are not known to it, it MUST ignore the header and SHOULD sent a TODO packet (vaguely similar to ICMP 'Parameter Problem' message) back to the sender. 

The last Header is always a SAOUDP_HEADER_LAST_HEADER header. Therefore, if there are no extensions, SAoUDP packet looks as **\| Header-Type=SAOUDP_HEADER_LAST_HEADER \| Data-Length=0 \| SAoIP-Payload \|**.

All Headers (except for LAST_HEADER, which is described below) have the following format: 

**\| Data \| Data-Length \| Header-Type \|**

where Header-Type is an Reverse-Encoded-Int<max=2> field, Data-Length is also a Reverse-Encoded-Int<max=2> field, and Data is a variable-length field which has Data-Length size.

Currently supported extensions are:

**\| Destination-IPv6 \| Data-Length=16 \| SAODUP_HEADER_AGGREGATE \|**

where Destination-IPv6 is a 16-byte field containing IPv6 address. The meaning and handling of Destination-IPv6 field is described in "SAoIP Aggregation and Destination-IPv6 field" section above.

**\| SAODUP_HEADER_LAST_HEADER \|**

SAODUP_HEADER_LAST_HEADER is always the last header in the header list. Indicates that immediately after this header, SAoIP-Payload field is located. Note that LAST_HEADER doesn't have a 'Data-Length' field.

SAoUDP and UDP
^^^^^^^^^^^^^^

SAoUDP packet uses UDP as an underlying transport; as such, it also (implicitly) contains standard 8-byte UDP headers as described in RFC 768. SAoUDP only uses unicast UDP. 

As we see it, SAoUDP (when used with the rest of the SmartAnthill Protocol Stack) is compliant with RFC5405 ("Unicast UDP Usage Guidelines for Application Designers"), and is therefore formally suitable for use in public Internet. However, for practical reasons (especially because of UDP-hostile firewalls, and because of not-properly-implemented or unsupported UDP NAT on many routers), use of SAoUDP on public Internet is discouraged. Use of SAoUDP in LANs or Intranets is perfectly fine (it is also fine for the Internet - that is, if you can make it work for your router/firewall).

SAoUDP Packet Sizes and Payloads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To comply with RFC 5405, SAoUDP SHOULD restrict maximum IP packet to the size of 576 bytes [1]_. Taking into account IP and UDP headers, it means that SAoUDP packet SHOULD be restricted to `576-60-8=508` bytes, and taking into account maximum size of supported SAoUDP headers, SAoIP-Payload for SAoUDP SHOULD be restricted to 508-TODO=TODO. This is a value which SHOULD be used for calculations of the maximum *Client_Side_SACCP_payload* as used in :ref:`saprotostack` document. For example, if SAoUDP payload size is typical TODO bytes (as calculated above), then corresponding maximum SASP payload is 463bytes+7bits, maximum SAGDP payload is 457 bytes, and maximum SACCP payload (and therefore *Client_Side_SACCP_payload*) is also TODO bytes.

.. [1] Strictly speaking, RFC 5405 says that MTU should be less than `min(576,first-hop-MTU)`; if first-hop-MTU on an interface which SmartAnthill Client uses, is less than 576, maximum SACCP payload SHOULD be recalculated accordingly; note that due to the block nature of SASP, dependency between SAoUDP payload and SACCP payload in not exactly linear and needs to be re-calculated carefully; however, MTU being less than 576 is very unusual these days.

