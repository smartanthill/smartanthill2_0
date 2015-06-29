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

:Version:   v0.3

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SAoIP is a part of SmartAnthill 2.0 protocol stack. It belongs to Layer 4 of OSI/ISO Network Model, and is responsible for transferring SAoIP payload (usually SASP packets) between SmartAnthill Client (normally implemented by SmartAnthill Core) and SmartAnthill Device or SmartAnthill Router.

Within SmartAnthill protocol stack, SAoIP is located right below SASP. 

.. contents::


SAoIP Flavours
--------------

Currently, only one flavour of SAoIP is supported: SAoUDP. In the future, SAoTCP and SAoTLSoTCP may be added, though their support won't be mandatory for SmartAnthill Devices. 

SAoIP Requirements
------------------

Generally, SAoIP is a very simple wrapper around SAoIP payload (which are normally SASP packets). As guaranteed delivery is normally handled by SAGDP, no guarantees are required (and neither are provided) by SAoIP in general. Even when/if certain SAoIP flavour (such as SAoTCP) provides certain delivery guarantees, SAoIP application layer (normally SASP+SAGDP+SACCP) MUST NOT rely on delivery guarantees provided by specific SAoIP flavour.

SAoIP SCRAMBLING
----------------

SCRAMBLING is an optional feature of SAoIP. SAoIP SHOULD use SCRAMBLING whenever SAoIP goes over non-secure connection; while not using SCRAMBLING is not a significant security risk, but might reveal some information about packet destination and/or might simplify certain DoS attacks. For the purpose, any connection SHOULD be considered as exposed (and therefore SCRAMBLING procedure SHOULD be used) unless proven secure; in particular, all connections which go over Wi-Fi or over the Internet, SHOULD be considered as exposed.

SAoIP uses SCRAMBLING procedure as described in :ref:`sascrambling` document. 

SCRAMBLING requires that both parties share the same symmetric key (for details, see :ref:`sascrambling` document). **This symmetric key MUST be completely independent and separate from any other keys, in particular, from SASP keys**. 

SAoIP SCRAMBLING uses Default SCRAMBLING-Header formatting schema as described in :ref:`sascrambling` document.

SCRAMBLING being optional
^^^^^^^^^^^^^^^^^^^^^^^^^

In some cases (for example, if all the communications is within Intranet without being passed through wireless links, or performed over TLS), SAoIP MAY omit SCRAMBLING procedure. In fact, if there is no information about SCRAMBLING key for the packet sender, both SmartAnthill Router and SmartAnthill IP-Enabled Device SHOULD try to interpret the packet as the one without SCRAMBLING applied. 

Formally, within SmartAnthill Protocol Stack omitting SCRAMBLING doesn't affect any security guarantees (as such guarantees are provided by SASP, which is not optional). However, as SCRAMBLING provides some benefits at a very low cost, by default SCRAMBLING procedure SHOULD be applied to all communications which are potentially exposed to the attacker.

SAoUDP
------

Unless SAoUDP packet is intended to be transferred over SAMP, it is formed as follows: 

* SAoUDP payload is SCRAMBLED
* it is placed over an UDP packet

SAoUDP and UDP
^^^^^^^^^^^^^^

SAoUDP packet uses UDP as an underlying transport; as such, it also (implicitly) contains standard 8-byte UDP headers as described in RFC 768. SAoUDP only uses unicast UDP. 

SAoUDP+UDP (compressed)
-----------------------

When SAoUDP packet is transferred over SAMP, it MUST be combined with UDP/IP packet information, and MUST be encoded (in 6LoWPAN-speak, "compressed") as follows:

**\| FOREIGN-IP-TYPE-AND-SOME-DATA \| OPTIONAL-FOREIGN-IP-DATA \| PAYLOAD \|**

where FOREIGN-IP-TYPE-AND-SOME-DATA is a Encoded-Unsigned-Int<max=2> bitfield substrate, described in detail below, OPTIONAL-FOREIGN-IP-DATA presence and length is defined by FOREIGN-IP-TYPE-AND-SOME-DATA (see below), and PAYLOAD is a payload of the upper protocol layer (usually SASP). Note that for over-SASP communications, payload is not SCRAMBLED (scrambling will be performed at SADLP-\* level).

"Foreign" address is either a source address (for packets travelling from Central Controller to Device), or destination address (for packets travelling from Device to Central Controller). Another address (non-"foreign" one) can always be derived from SAMP headers and is never transferred at this level.

If bit[0] of FOREIGN-IP-TYPE-AND-SOME-DATA is 0, then:

* foreign IP address is an address within the current PAN, bits [1..] of FOREIGN-IP-TYPE-AND-SOME-DATA represent SAMP address. As a consequence, for SmartAnthill Controller's FOREIGN-IP-TYPE-AND-SOME-DATA is encoded as a single byte 0x00.

If bit[0] of FOREIGN-IP-TYPE-AND-SOME-DATA is 1, then: 

* if bits[1..] = 0, then foreign IP address is an IPv4 address, and OPTIONAL-FOREIGN-IP-DATA is 4-byte IPv4 address (encoded as described in https://en.wikipedia.org/wiki/IPv4). It is normally translated to an IPv6 address using SIIT (see https://en.wikipedia.org/wiki/IPv6_transition_mechanism).
* if bits[1..] = 1, then foreign IP address is a full IPv6 address, bits [2..] MUST be zero, and OPTIONAL-FOREIGN-IP-DATA is 16-byte IPv6 address (encoded as described in https://en.wikipedia.org/wiki/IPv6).
* if bits[1..] = 2, then foreign IP address is 64 lower bits of IPv6 address, bits [2..] MUST be zero, and OPTIONAL-FOREIGN-IP-DATA is 8-byte (remaining 64 bits of IPv6 address being the same as IPv6 address of the SmartAnthill Router).
* other values of bits[1..6] (when bit[0] = 1) are RESERVED.

SmartAnthill Router
-------------------

SmartAnthill Router is responsible for converting packets from "SAoUDP over UDP" format which travels over the IP network, into "compressed SAoUDP+UDP" format which travels over SmartAnthill PAN (and which can be seen as a compression which is similar to 6LoWPAN, but re-optimized for SmartAnthill needs). After conversion, the packet is sent over SAMP. On the way back (from Device to IP network), Router receives packet over SAMP, converts it into "SAoUDP over UDP" format which travels over the IP network, and sends it over IP network.

Currently, SmartAnthill Router supports only stateless convertion/compression. If necessary, stateful conversion/compression may be added in the future.

In general, SmartAntill Router can operate either at application level, or at L3 level. Currently, only application-level SmartAnthill Router is implemented.

SmartAnthill Application-Level Router
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to packet format conversion described above, SmartAnthill Application-Level Router allows to perform NAT and/or PAT. 

SmartAnthill Router keeps the following records in SmartAnthill Database (SA DB) table DEVICE_MAPPINGS: 

**\| Device-Key-ID \| IPv6 \| SAoIP-Flavour \| port \| SCRAMBLING-Key \| Bus ID \| Intra-Bus ID \| Recrypt-External-Key \| Recrypt-Internal-Key \|**

In addition, there is another SA DB table KEY_MAPPINGS:

**\| Device-Key-ID \| external-SASP-key-ID \| internal-SASP-key-ID \|**

When an incoming SAoIP packet comes in (from a receiving socket), SmartAnthill Router: 

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
* forms "SAoUDP+UDP (compressed)" packet as decsribed above, using SASP 'hacked' packet as a payload
* forms SAMP packet, and then SADLP-\* packet (depending on the bus in use) as described in respective documents, using "SAoUDP+UDP (compressed)" packet as a payload
* sends SADLP-\* packet to (Bus-ID, Intra-Bus-ID)

When an incoming packet from SADLP-\* comes in (from certain Bus-ID and Intra-Bus-ID), SmartAnthill Router:

* processes SADLP-\* incoming packet to obtain SAMP packet, and then "SAoUDP+UDP (compressed)" as described in respective documents
* processes "SAoUDP+UDP (compressed)" packet as described above, to obtain PAYLOAD and FOREIGN-IP-ADDRESS
* parses PAYLOAD to get SASP packet, and gets key-ID out of it (this can be done without decrypting SASP packet); for SmartAnthill Router, this is internal-SASP-key-ID
* finds a row in DEVICE_MAPPINGS table, based on (Bus ID, Intra-Bus ID), and obtains Device-Key-ID and SCRAMBLING-Key TODO: what to do if not found
* finds a row in KEY_MAPPINGS table, based on (Device-Key-ID, internal-SASP-key-ID), and obtains external-SASP-key-ID TODO: what to do if not found
* changes ('hacks') SASP packet to use external-SASP-key-ID instead of internal-SASP-key-ID; this can be done without decrypting SASP packet
* if DEVICE_MAPPINGS record found above, contains "re-crypt" information, SmartAnthill Router decrypts SASP packet within SAoIP-Payload (using Recrypt-Internal-Key) and encrypts it again (using Recrypt-External-Key)
* forms a SAoIP packet, using FOREIGN-IP-ADDRESS, and 'hacked' SASP packet as a payload
* if SCRAMBLING-Key is not NULL, SCRAMBLES packet, using SCRAMBLING-Key
* sends packet to FOREIGN-IP-ADDRESS

