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

.. _siot_dlp_btle_hci:

SimpleIoT Data Link Protocol over Bluetooth Low Energy HCI (SimpleIoT/DLP-BTLE-HCI)
===================================================================================

:Version:   0.1.1a

*NB: this document relies on certain terms and concepts introduced in* :ref:`siot` * and * :ref:`siot_hmp` *documents, please make sure to read it before proceeding.*

This document defines SimpleIoT/DLP-BTLE-HCI protocol. SimpleIoT/DLP-BTLE-HCI operates on top of HCI as defined for Bluetooth Low Energy. Levels of SimpleIoT/DLP-BTLE-HCI which are lower than HCI (in particular, baseband controller and LMP) are the same as for Bluetooth Low Energy; however, layers of SimpleIoT/DLP-BTLE-HCI which are higher than HCI (L2CAP and up), are not compatible with Bluetooth Low Energy. This approach allows to use HCI-compatible Bluethooth Low Energy hardware, while keeping complexity of upper Bluetooth layers (which functionality is unnecessary for SimpleIoT stack) out of scope. 

SimpleIoT/DLP-BTLE-HCI as a TERMINAL-ADVERTISING protocol
---------------------------------------------------------

SimpleIoT/DLP-BTLE-HCI is a TERMINAL-ADVERTISING protocol, as defined in :ref:`siot_hmp` document. It means that SimpleIoT/DLP-BTLE-HCI needs to support at least the following operations:

* "advertising" with some data attached to "advertising"
* establishing connection

  + after connection is established, there MUST be a way to find out that it is broken (at least when the packet is sent over the connection, ideally - even when there are no packets)

* transmitting data packets over the established connection

MAC Addresses
-------------

Due to software nature of SimpleIoT/DLP-BTLE-HCI, whenever MAC address is required by HCI equipment manufacturer, it MUST be generated randomly out of the following "Locally Administered MAC Addresses" (during either "Zero Pairing", or crypto-safe RNG as defined in :ref:`siot_rng`): 

X6-XX-XX-XX-XX-XX

where each 'X' represents a random hex digit. 

*Side Note: in general, X2-\*, XA-\*, and XE-\* addresses are also "Locally Administered MAC Addresses", but we've chosen to use only X6-\* for now.*

Advertising
-----------

TERMINAL-ADVERTISING "advertising" in SimpleIoT/DLP-BTLE-HCI is implemented as BT-LE "advertising" (using HCI_LE_Set_Advertising_Parameters/HCI_LE_Set_Advertising_Data/HCI_LE_Set_Advertise_Enable HCI commands). The packet always has a payload which consists of a header, plus a Hmp-To-Santa SimpleIoT/HMP packet (which may have empty payload, as described in :ref:`siot_hmp` document). If current outstanding packet directed to Root, is too large for BT-LE "advertising" (including SCAN_RSP), a packet with an empty Hmp-To-Santa payload MUST be "advertised", and outstanding packet SHOULD be postponed until the moment when connection is established.

BT-LE Advertising SHOULD be performed over all available advertising channels (normally there are three of them), using "Connectable Undirected" advertising event type, with advertising interval normally set to 100ms (TODO - acceptable variations and exponential increase with time). Payload of BT-LE advertising for SimpleIoT/DLP-BTLE-HCI has the following format:

**\| FLAGS \| Hmp-Packet \|**

where FLAGS is an Encoded-Unsigned-Int<max=2> bitfield substrate, with bit[0] being HAVE_OUTSTANDING_DATA flag, and other bits reserved (MUST be 0). If there is an outstanding packet which is too small to fit into single BT-LE "advertising" (ADV_IND) packet, but is large enough to fit into BT-LE "advertising" packet + BT-LE "scan response" (SCAN_RSP) packet, Device MAY either to postpone the outstanding packet until the moment when connection is established, or to use SCAN_RSP (via HCI_LE_Set_Scan_Response_Data command) to transfer the whole outstanding packet before establishing connection. BT-LE advertising filters are not normally used.

If Device has just lost an upstream BT-LE connection, and has no reasons to believe that last-known-good upstream connection is broken (single Disconnect is not such a reason), Device SHOULD try using BT-LE "direct advertising", indicating last-known-good Retransmitting Device as an intended recipient; as with indirect advertising, packet payload MAY be zero if the packet doesn't fit into BT-LE Advertising limits. In the case of "direct advertising", packet will normally be different from To-Santa; if Retransmitting Device receives such a "direct advertising" packet, it simply re-establishes BT-LE connection and passes the packet without initiating any routing changes. However, if "direct advertising" doesn't lead to a BT-LE Connection in TODO seconds, Device MUST switch to "indirect advertising" as described above.

"Listening" on Retransmitting Devices
-------------------------------------

Retransmitting Devices MUST be always-on, in a sense that they MUST always constantly scanning (using HCI_LE_Set_Scan_Parameters/HCI_LE_Set_Scan_Enable commands and HCI_LE_Advertising_Report_Event). Normally, scan interval SHOULD be set to 125 ms, and scan window SHOULD be set to 100ms (TODO - acceptable variations). BT-LE scanning filters are not normally used.

Establishing Connection
-----------------------

As described in :ref:`siot_hmp` document, whenever the Root has decided on a route and updated routing table on the Retransmitting Device which is next to not-connected-yet-Device, the Retransmitting Device needs to establish connection with the not-connected-yet-Device. In SimpleIoT/DLP-BTLE-HCI, it is done in the following way:

* Retransmitting Device continues to scan (as described above).
* On receiving next "advertising" packet from not-connected-yet-Device, it initiates connection (by sending CONNECT_REQ BT-LE packet, which is done using HCI_LE_Create_Connection command on Retransmitting Device, TODO: will there be a HCI_LE_Connection_Complete_Event on Retransmitting Device?)
* On not-connected-yet-Device, on receiving CONNECT_REQ BT-LE packet (using HCI_LE_Connection_Complete_Event), connection is considered established

In SimpleIoT/DLP-BTLE-HCI, BT-LE Connections SHOULD have the following parameters:

* connInterval=100ms
* connSlaveLatency=0 (TODO: increase when waiting for Slave to transmit?)
* connSupervisionTimeout=5s. When connSupervisionTimeout is exceeded, Controller is expected to report HCI_Disconnection_Complete_Event.
* transmitWindowOffset SHOULD be set to 0 if there is outstanding data within the channel (on either side of the connection, taking into account HAVE_OUTSTANDING_DATA flag from most recent "Advertising" packet from the target Device), and to connInterval otherwise.
* transmitWindowSize=5ms (TODO - are we sure?).

After this point, BT-LE connection is considered established.

Transmitting Data Packets
-------------------------

Upper-layer data packets (normally SimpleIoT/HMP packets) are transmitted over SimpleIoT/DLP-BTLE-HCI as HCI Data Packets; all the BT-LE payloads MUST be at most 27-bytes long (as demanded by BT-LE specification); if a larget packet needs to be transferred, it MUST be split into several "chunk" packets with each having at most 27-byte payload; then  "chunks" MUST be transferred over HCI one-by-one, with first "chunk" having a "Packet Boundary" flag, and the rest not having this flag (see also discussion on the flags in section 8.3.4 of "Bluetooth Low Energy. The Developer's Handbook" by Robert Heydon). 

At the physical level, this should result in the following: whenever the BT-LE "connection event" comes, "chunks" with and without data will go back and forth over the BT-LE connection, transferring the data in both directions.

Disconnect
----------

Before turning off it's transmitter, Device, if it has a BT-LE connection, SHOULD disconnect the connection (using HCI_Disconnect command).

If BT-LE connection to Master is dropped for any reason (which should be indicated by HCI_Disconnection_Complete_Event), then Device MUST turn into Advertising mode as described above.

Scrambling
----------

TODO

HCI Flow Control
----------------

Both HCI Data Flow Control and HCI Command Flow Control mechanisms MUST be observed by compliant SimpleIoT/DLP-BTLE-HCI implementations.

