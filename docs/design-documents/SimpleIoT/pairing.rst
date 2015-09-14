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

.. _siot_pairing:

SimpleIoT Pairing
=================

:Version:   v0.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`siot` *document, please make sure to read it before proceeding.*

"Pairing" SimpleIoT Device to SimpleIoT Client is necessary to ensure secure key exchange between SimpleIoT Device and SimpleIoT Client. As soon as "pairing" is completed, both parties have a 128-bit symmetric key shared between them, and can use it for SimpleIoT/SP purposes.

SimpleIoT Pairing comes in several flavours. SimpleIoT Device MUST implement at least one of these flavours. SimpleIoT Client MUST implement *all* these flavours. 

SimpleIoT Pairing flavours are divided into two categories: Zero Pairing (which doesn't involve communication over SimpleIoT communication channel), and Over-the-Air (OtA) pairing. 

SimpleIoT Zero Pairing
----------------------

Zero pairing doesn't involve communication over SimpleIoT communication channel.

SimpleIoT Zero Programming Pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SimpleIoT Zero Programming Pairing applies only to those devices which can be completely reprogrammed. It is a RECOMMENDED way of pairing for hobbyist-oriented SimpleIoT Devices.

SimpleIoT Zero Programming Pairing consists of:

* Client generating secret key (see TODO for details)
* Client preparing (e.g. compiling or linking) a program which includes generated secret key as static data
* Client storing generated secret key in Client DB
* Client programming Device using prepared program (which contains generated secret key)

TODO: restrictions on SimpleIoT Device programming-socket key access.

SimpleIoT Zero Paper Pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SimpleIoT Zero Paper Pairing MAY be used by those mass-market-oriented SimpleIoT Devices, for which implementing other pairing methods is not feasible. **Zero Paper Pairing SHOULD NOT be used if other pairing methods are feasible. Zero Paper Pairing MUST NOT be used by SimpleIoT Security Devices unless it is demonstrated that other pairing methods are not feasible for the Device.**

Zero Paper Pairing requires each Device to:

* have unique 128-bit crypto-random key programmed in as it's SimpleIoT/SP AES key
* have this 128-bit key printed in the following user-friendly form:

  + 128-bit key is converted to a large unsigned integer (using SimpleIoT Endianness) from 0 to 2^128-1
  + this large unsigned integer is written as an integer using base 36 (i.e. using 36 digits in each position); to write digits 0-9 in this representation, symbols '0'-'9' are used; to write digits 10-35 in this representation, symbols 'A'-'Z' (upper case) are used. This representation will have at most 25 symbols (as 36^25 > 2^128); if there are less symbols than 25, they're left-padded with zeros to 25
  + these 25 symbols are written in dash-separated groups of five
  + checksum symbol is calculated as a modulo-36 sum of all the symbols
  + checksum is appended (via dash) to dash-separated groups of five, forming XXXXX-XXXXX-XXXXX-XXXXX-XXXXX-X pattern
  + for example, all-zero key will be written as 00000-00000-00000-00000-00000-0

* this user-friendly form of 128-bit crypto-random key MUST be provided in a printed form with the device.

In addition, to ensure that if the printed key is lost, Device is still usable, Devices using Zero Paper Pairing, MUST comply to the following **Reprogramming Requirements**:

* Device MUST provide an option to re-program key, using either UART or USB. **Wireless programming methods are expressly forbidden; in addition, any way (whether wired or wireless) to read the key from the device, is expressly forbidden**. 

  + this can be made by one of the following methods:

    - full Device reprogramming; to be compliant, it MUST be done as follows:
      
      * Device MUST be re-programmable using Platform.IO
      * Manufacturer MUST provide source code for the Device programming in a form which is used by SimpleIoT for programming of hobbyist-orineted SimpleIoT Devices

        + this source code MUST be available free of charge BOTH from manufacturer's web site, AND from an allowed third-party repository. List of allowed third-party source code repositories TBD (github, sourceforge, something else?)

    - key reprogramming. Protocols for key reprogramming over UART and over USB are TBD.

SimpleIoT OtA Pairing
---------------------

SimpleIoT OtA pairing provides security (including MITM protection) with minimal complexity involved.

From OtA Pairing perspective, SimpleIoT Device can be in one of the following OtA pairing states: 

* PRE-PAIRING
* PAIRING-MITM-CHECK
* PAIRING-COMPLETED

IMPORTANT: Change from any of the states into PRE-PAIRING state MUST be implemented ONLY via physical manipulations of end-user with SimpleIoT Device (and MUST NOT be allowed remotely). Examples of valid user interfaces to perform such a change include on-Device button or buttons (for example, if two buttons are simultaneously kept pressed for over N seconds) and on-Device PCB jumper. When changing Device state to PRE-PAIRING state, state of Device RNG (i.e. data used for random number generation) MUST NOT be affected.

In PRE-PAIRING state, SimpleIoT/SP MUST use 'zero' AES-128 key (with AES key consisting of all zeros). 

In PRE-PAIRING state, no programs are allowed to be sent to SimpleIoT/CCP; only TODO SimpleIoT/CCP packets are allowed. In PAIRING-MITM-CHECK state, SimpleIoT/CCP programs are allowed; however, in this state, SimpleIoT/CCP restricts EXEC command of SimpleIoT/VM to the only Built-In bodypart (id=BUILTIN_BODYPART_PAIRING). 

From security perspective, SimpleIoT OtA pairing works as follows:

* BOTH parties generate DH randoms (`a` and `b` - 1024- or 2048-bit ones). 
* parties perform anonymous Diffie-Hellman key exchange, obtaining a 1024- or 2048-bit shared secret Z.
* parties derive 128-bit key K and 128-bit verification value X out of Z.
* from this point on, on both sides SimpleIoT/SP starts to use key K, as SimpleIoT/SP AES key
* parties use verification value X (which is essentially a MITM check key) to perform MITM protection check depending on the OtA pairing flavour. During this exchange, Device is kept in PAIRING-MITM-CHECK Device OtA pairing state. 
* if MITM protection check indicates that everything is fine - Device OtA pairing state is changed to PAIRING-COMPLETED, and normal work can be started.

Pre-Programmed Keys and RNGs
----------------------------

It should be understood that to ensure security, Devices MUST comply to at least one of the following two requirements:

* each device MUST have unique pre-programmed SimpleIoT/SP key:
  
  + this applies to Zero Pairing Devices

or

* Device MUST implement a cryptographic RNG, as described in :ref:`siot_rng` document

  + this applies to OtA Pairing Devices
  + one way of implementing cryptographic RNG is "Poor-Man's RNG" with a pre-initialized key (see :ref:`siot_rng` document for details)
  + another way of implementing cryptographic RNG is "Fortuna" as described is :ref:`siot_rng` document

    - as described in :ref:`siot_rng`, initialization of "Fortuna" cryptographic RNG MAY require obtaining additional entropy; exact way of doing it is described in 'Entropy Gathering' procedure below

SimpleIoT OtA Pairing Protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All the messages within one pairing procedure form a single "packet chain". That is, "packet chain" for a normal OtA Pairing exchange works as follows:

**Pairing-Ready-Pseudo-Response - Pairing-Pre-Request - Pairing-Pre-Response - Pairing-DH-Data-Request - Pairing-DH-Data-Response - ... - Pairing-DH-Data-Request - Pairing-DH-Data-Response**

When both sides receive the last of Pairing-DH-DATA-\* packets (the ones which provide the whole DH data, with size defined according to KEY-EXCHANGE-TYPE field in Pairing-DH-Data-Request), they proceed with calculation of SimpleIoT/SP key.

"Awaiting pairing" mode
'''''''''''''''''''''''

To avoid Device connecting to wrong SimpleIoT Client, SimpleIoT Client MUST NOT proceed with "pairing" in response to Pairing-Ready packets unless SimpleIoT Client is in "awaiting pairing" mode. "Awaiting pairing" mode for SimpleIoT Client MUST be user-initiated, and MUST NOT be kept for longer than 1 hour, unless user requests another "awaiting pairing". This is necessary to reduce "partially paired to wrong SimpleIoT Client" encounters (which MUST have a way to be handled separately; TODO: example in SimpleIoT/RF).

TODO: errors (Z=1 per NIST SP 800-56B, and derived-key=0 to avoid being caught by attacks on misimplementations)!

OtA Pairing Protocol Packets
''''''''''''''''''''''''''''

Pairing-Ready-Pseudo-Response: **|\ ENTROPY-NEEDED-SIZE \|**

where ENTROPY-NEEDED-SIZE is an Encoded-Unsigned-Int<max=2> field specifying amount of needed entropy in bytes.

Pairing-Ready-Pseudo-Response is not really a response, but a request from Device side which initiates pairing sequence. It is sent as a payload for a SIOT-CCP-OTA-PAIRING-RESPONSE message (which in turn initiates a new "packet chain"), with 2 "additional bits" being 0x0. If ENTROPY-NEEDED-SIZE is not zero, it indicates that Phase 1 of 'Entropy Gathering Procedure' (see below) is necessary before issuing a Pairing-Pre-Request from Client side.

If Client is not in "awaiting pairing" mode, it MUST respond with Pairing-Error-Request with ERROR-CODE = ERROR_NOT_AWAITING_PAIRING.

Pairing-Pre-Request: **\| OTA-PROTOCOL-VERSION-NUMBER-MAJOR \| OTA-PROTOCOL-VERSION-NUMBER-MINOR \| CLIENT-RANDOM \| PROJECTED-NODE-ID \| CLIENT-OTA-AND-SIOT-SP-CAPABILITIES \|**

where OTA-PROTOCOL-VERSION-NUMBER-\* are Encoded-Unsigned-Int<max=2> fields, CLIENT-RANDOM is a 16-byte field with crypto-random data, PROJECTED-NODE-ID is an Encoded-Unsigned-Int<max=2> field, containing NODE-ID which Client intends to assign to the Device if pairing is successful, and CLIENT-OTA-AND-SIOT-SP-CAPABILITIES TBD. 

Pairing-Pre-Request is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-REQUEST message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-REQUEST message being 0x0.

Pairing-Pre-Response: **\| ENTROPY-NEEDED-SIZE \| OPTIONAL-DEVICE-RANDOM \| OPTIONAL-DEVICE-BUS-TYPE \| OPTIONAL-DEVICE-INTRABUS-ID-SIZE \| OPTIONAL-DEVICE-INTRABUS-ID \| OPTIONAL-DEVICE-OTA-AND-SIOT-SP-CAPABILITIES \|**

where ENTROPY-NEEDED-SIZE is an Encoded-Unsigned-Int<max=2> field, OPTIONAL-DEVICE-RANDOM is an optional 32-byte field, OPTIONAL-DEVICE-BUS-TYPE is an Encoded-Unsigned-Int<max=1> field representing a enum of bus types (TBD), OPTIONAL-DEVICE-INTRABUS-ID-SIZE is an Encoded-Unsigned-Int<max=1> field, representing size of OPTIONAL-DEVICE-INTRABUS-ID field in bytes, OPTIONAL-DEVICE-INTRABUS-ID depends on the bus type, and OPTIONAL-DEVICE-OTA-AND-SIOT-SP-CAPABILITIES (format TBD); all the OPTIONAL-\* fields are present only if this Pairing-Pre-Response packet is the first such packet in current "pairing" exchange.

Pairing-Pre-Response is sent as a payload for a SIOT-CCP SIOT-CCP-OTA-PAIRING-RESPONSE message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-RESPONSE message being 0x1.

NB: to comply with key generation requirements as specified in :ref:`siot_rng` document, Device MUST request at least amount of entropy which is equal to the `b` parameter size for DH key exchange; however, Device MAY request more entropy (up to 256 extra bytes per pairing attempt, which requests MAY be split into packets as small as 1-byte) - for example, to initialize it's own Fortuna generator. 

If ENTROPY-NEEDED-SIZE is not zero, it means that "Entropy Gathering" Phase 3 is necessary (see below), and that Client MUST reply with a Pairing-Entropy-Provided-Request.

Pairing-Entropy-Provided-Request: **\| ERROR-CODE \| ENTROPY \|**

where ERROR-CODE is an Encoded-Unsigned-Int<max=2> field, equal to zero, and ENTROPY is an arbitrary-length field with cryptographically safe random data. 

Pairing-Entropy-Provided-Request is sent as a payload for a SIOT-CCP SIOT-CCP-OTA-PAIRING-REQUEST message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-REQUEST message being 0x1. Note that "additional bits" for Pairing-Entropy-Provided-Request are the same as for Pairing-Error-Request, and they're distinguished by the value of ERROR-CODE field.

Client MAY supply less entropy than it was requested (and SHOULD do it in case if requested data potentially exceeds MTU); in such a case, Device SHOULD request more entropy via replying with an appropriate message with a non-zero ENTROPY-NEEDED-SIZE.

In response to Pairing-Entropy-Provided-Request, Device MUST send another Pairing-Ready-Pseudo-Response or Pairing-Pre-Response packet (depending on the Phase of Entropy Gathering procedure currently in progress), specifying non-zero ENTROPY-NEEDED-SIZE if it still has not enough entropy. 

Pairing-Error-Request: **\| ERROR-CODE \|**

where ERROR-CODE is an Encoded-Unsigned-Int<max=2> field, never equal to zero. 

Pairing-Error-Request is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-REQUEST message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-REQUEST message being 0x1. Note that "additional bits" for Pairing-Error-Request are the same as for Pairing-Entropy-Provided-Request, and they're distinguished by the value of ERROR-CODE field.

Pairing-DH-Data-Request: **\| OPTIONAL-KEY-EXCHANGE-TYPE \| DH-REQUEST-PART \|**

where OPTIONAL-KEY-EXCHANGE-TYPE is sent only for the very first Pairing-DH-Data-Request within the "pairing", and is Encoded-Unsigned-Int<max=2> field with values defined below, and DH-REQUEST-PART is a field taking the rest of the packet, and representing first remaining (SimpleIoT-Endianness-wise) bytes of `A = g^a mod p` from DH key exchange (using SimpleIoT Endianness).

Supported OPTIONAL-KEY-EXCHANGE-TYPEs:

* value 0:

  + Key Exchange: DH with 1024-bit MODP group with 160-bit Prime Order Subgroup as defined in RFC 5114. This OPTIONAL-KEY-EXCHANGE-TYPE MUST NOT be used for Security SimpleIoT Devices. *NB: MODP groups from RFC 5114 are preferred to earlier-defined ones (for example, those from RFC 3526), as they explicitly comply with NIST-suggested restrictions, in particular, restrictions on q.*
  + Key Derivation: SHA256-based

* value 1:

  + Key Exchange: DH with 2048-bit MODP group with 256-bit Prime Order Subgroup as defined in RFC 5114.
  + Key Derivation: SHA256-based

* others: MAY be added as necessary

TODO: double-check presence of any typical patterns in Z, and decide on split (first-half/second-half or even-bits/odd-bits)

Pairing-DH-Data-Request is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-REQUEST message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-REQUEST message being 0x2.

Pairing-DH-Data-Response: **\| DH-RESPONSE-PART \|**

where DH-RESPONSE-PART is a field taking the whole packet; length of DH-RESPONSE-PART MUST be exactly the same as DH-REQUEST-PART in the incoming Pairing-DH-Data-Request message. DH-RESPONSE-PART represents first remaining (SimpleIoT-Endianness-wise) bytes of `B = g^b mod p` from DH key exchange (using SimpleIoT Endianness).

Pairing-DH-Data-Response is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-RESPONSE message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-RESPONSE message being 0x2.

Pairing-Ok-Request: **\| OK-A-ENTROPY-CHECKSUM \| NODE-ID \|**

where OK-A-ENTROPY-CHECKSUM is a 16-byte field containing result of SimpleIoT/SP-tag(nonce=(varying-part=1,direction=from-client-to-device),authenticated-data=All-Sent-ENTROPY-Combined,key=derived-SimpleIoT/SP-key), where nonce is constructed in the same way it is constructed in SimpleIoT/SP, and NODE-ID is an Encoded-Unsigned-Int<max=2> field containing SimpleIoT/MP node ID to be assigned to the Device. NODE-ID is conditional on OK-A-ENTROPY-CHECKSUM check described below, otherwise NODE-ID MUST be ignored.

Pairing-Ok-Request is sent by Client when the last Pairing-DH-Data-Response is received; it is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-REQUEST message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-RESPONSE message being 0x3.

On receiving Pairing-Ok-Request, Device calculated it's own DEVICE-OK-A-ENTROPY-CHECKSUM with derived-SimpleIoT/SP-key, compares it to received OK-A-ENTROPY-CHECKSUM. If the check is Ok, then Device calculates OK-B-ENTROPY-CHECKSUM (the same way as OK-A-ENTROPY-CHECKSUM is calculated, but with direction=from-device-to-client), and sends it back as a part of Part-Ok-Response; then Device changes pairing state into Pairing-MITM-Check, sets SimpleIoT/SP key to derived-SimpleIoT/SP-key for all future communications with Client, and sets next SimpleIoT/SP nonce varying-part (including the one stored in persistent storage) to 2.

If DEVICE-OK-A-ENTROPY-CHECKSUM and received OK-A-ENTROPY-CHECKSUM don't match - Device MUST switch back to PRE-PAIRING state and report TODO error to the Client.

Pairing-Ok-Response: **\| OK-B-ENTROPY-CHECKSUM \|**

where OK-B-ENTROPY-CHECKSUM is a 16-byte field.

Pairing-Ok-Response is sent as a payload for a SimpleIoT/CCP SIOT-CCP-OTA-PAIRING-RESPONSE message, with 2 "additional bits" for SIOT-CCP-OTA-PAIRING-RESPONSE message being 0x3.

On receiving Pairing-Ok-Response, Client calculates it's own CLIENT-OK-B-ENTROPY-CHECKSUM, compares it with received OK-B-ENTROPY-CHECKSUM. If everything is fine - "pairing" can be considered completed, and Client sets SimpleIoT/SP key (to be used by SimpleIoT/SP) to derived-SimpleIoT/SP-key for all future communications with this Device, and sets next SimpleIoT/SP nonce varying-part (including the one stored in persistent storage) to 2. After that, Client starts to perform MITM check (using MITM-Check-Program as described below).

If CLIENT-OK-B-ENTROPY-CHECKSUM and received OK-B-ENTROPY-CHECKSUM don't match - Client reports end-user a potential attack on pairing (without such an attack, chances of ENTROPY-CHECKSUM mismatching are on the order of 2^-120), and asks end-user to re-start pairing by manually switching Device to PRE-PAIRING state (using appropriate UI as described above).

Entropy Gathering
'''''''''''''''''

In some cases, as a prerequisite for Device to be able to perform pairing, RNG needs to be supplied with entropy (exact conditions are described in :ref:`siot_rng` document); NB: as described in :ref:`siot_rng`, entropy usually needs to be supplied not only to the first pairing of the Device, but also to any subsequent pairing.

The procedure of Entropy Gathering is performed as follows:

Phase 1 (OPTIONAL, used only if Device ID needs to be generated, hardware-assisted Fortuna PRNG is used, and Fortuna doesn't have enough entropy):

* Device sends Pairing-Ready-Pseudo-Response with non-zero ENTROPY-NEEDED-SIZE
* Client replies with Pairing-Entropy-Provided request, sent as a broadcast (SHOULD be restricted to those Retransmitting Nodes which may reach the Device)
* this Pairing-Ready-Pseudo-Response - Pairing-Entropy-Provided sequence is repeated until Device has sufficient entropy to generate Device ID (this is the same as for regular "pairing", as described in :ref:`siot_rng` document)
* NB: during Phase 1, Pairing-Entropy-Provided packets from Client to Device are sent as SimpleIoT/MP From-Santa packets (see :ref:`siot_mp`) which do not distinguish between target Devices, so there is a chance that more than one Device obtains the same packet. However, these same packets will (with an overwhelming probability) lead to different states within Fortuna PRNGs on different Devices, which will allow to distinguish these (originally potentially indistinguishable) Devices.

Phase 2:

* Device sends Pre-Pairing-Response non-zero ENTROPY-NEEDED-SIZE, DEVICE-ID-FLAG set, and all Device ID-related fields.
* Client replies with Pairing-Entropy-Provided request
* NB: starting from Phase 2, all the packets from Client to Device are sent as SimpleIoT/MP Unicast packets (see :ref:`siot_mp`) and are addressed to specific Device (using Device ID from Phase 2).

Phase 3:

* Device sends non-zero ENTROPY-NEEDED-SIZE, and DEVICE-ID-FLAG not set
* Client replies with Pairing-Entropy-Provided request
* Device processes received entropy as described in :ref:`siot_rng` document
* the process described in Phase 3 is repeated until Device has sufficient entropy (as defined in :ref:`siot_rng` document)

It should be noted that number of packets sent and received is IMPORTANT for security purposes, so combining packets contrary to requirements in :ref:`siot_gdp` is strictly prohibited.

DH Random Generation
''''''''''''''''''''

For both Client side and Device side, DH random numbers (`a` and `b` respectively) MUST be generated as described in `Key Generation` section in :ref:`siot_rng` document.

SimpleIoT/SP Key Derivation
'''''''''''''''''''''''''''

When both sides have all the information they need (that is, Client has full `B = g^b mod p` and Device has full `A = g^a mod p`), they need to calculate shared secret Z (`Z = A^b mod p` for Device, and `Z = B^a mod p` for Client), and generate SimpleIoT/SP Key K (128 bit), as well as verification value X (also 128 bit), from Z.

SimpleIoT/SP Key K and verification value X are calculated as follows:

* for SHA256-based derivation: `K = SHAd256(Z||Info||first-half-of-CLIENT-RANDOM||first-half-of-DEVICE-RANDOM)`, `X = SHAd256(Z||Info||second-half-of-CLIENT-RANDOM||second-half-of-DEVICE-RANDOM)`, where Info='"SimpleIoT/SP"||KEY-EXCHANGE-TYPE||'K'-or-'X'||ROOT-NODE-ID||PROJECTED-NODE-ID' (where ROOT-NODE-ID is always 0, and 'K'-or-'X' is equal to 'K' ASCII byte if calculating 'K', and to 'X' ASCII byte if calculating 'X'). SHAd256(m) is SHA256(SHA256(m)), same as in [Fortuna]. *NB: this method differs from recommended by NIST, in that we're deriving both K and X from the same DH keys; as some function of X is exposed (via LED blinking), in theory it might leak some information about K; however, in practice we don't see any specific attack vectors (especially as obtaining key material from X requires reverting SHAd256, AND as blinking is not just X, but X-encrypted-with-a-random-key-which-is-transferred-over-encrypted-channel, so X itself is not easily accessible). We could use method of obtaining X which is similar to Simple Secure Pairing, but at the point we do not see it necessary.*
* other methods MAY be added in the future

OtA Pairing MITM-Check Program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After initial "packet chain" consisting of Pairing Request and Pairing Response, Device goes into PAIRING-MITM-CHECK state; MITM check is performed via "MITM-Check Program". 

MITM-Check Program is pretty much a regular SimpleIoT/VM program which goes over SimpleIoT/CCP (normally over SimpleIoT/GDP over SimpleIoT/SP). There is a difference from regular program though: MITM-Check Program MUST come only in PAIRING-MITM-CHECK Device pairing state. In this state, SimpleIoT/CCP (and/or SimpleIoT/VM) prohibits program to access any bodyparts, except for a Built-In bodypart with id=BUILTIN_BODYPART_PAIRING. This also ensures that despite there can be two bodyparts accessing the same LED (one is 'pairing' bodypart, another is regular bodypart), there is no possible conflict between the two. 

OtA Pairing Flavours
^^^^^^^^^^^^^^^^^^^^

All OtA Pairing Flavours run on top of SimpleIoT OtA Pairing Protocol, and differ only in their MITM-Check Programs.

SimpleIoT OtA Single-LED Pairing
''''''''''''''''''''''''''''''''

SimpleIoT OtA Single-LED Pairing is pairing mechanism, which is semi-automated (i.e. user is not required to enter any data, but will be required to position devices in a certain way), and which requires absolute minimum of resources on the Device side. Namely, all the Device needs to have (in addition to MCU) is one single LED. This LED MAY be any of existing LEDs on the Device. 

MITM-Check for Single-LED Pairing is performed as follows:

* User is asked to bring Device close to the webcam which is located on SimpleIoT Central Controller
* Client sends a MITM-Check program which requests LED to blink, using `Blinking-Function(random-nonce-sent-by-Client)=AES(key=verification-value-X,data=random-nonce-sent-by-Client)` as a blinking pattern. TODO: Built-in Plugin to produce AES(...) reply.
* Accordingly, Device starts blinking the LED
* Client, using webcam, recognizes blinking pattern and makes sure that it matches expectations.
* If expectations don't match, program may be repeated with a *different* random-nonce-sent-by-Client
* If expectations do match, another program (also technically a MITM-Check program) is sent to change OtA Pairing State of the Device to PAIRING-COMPLETED.

NB: SimpleIoT Client SHOULD support using webcam on a smartphone camera for "pairing" purposes (provided that TODO requirements for securing communication between SimpleIoT Client and smartphone's app, are met).

MITM-Check for Single-LED Pairing being User-OPTIONAL
#####################################################

All SimpleIoT Devices using Single-LED Pairing, MUST implement proper MITM Check procedures as described above. However, devices which are not designated as Security Devices, MAY set PAIRING-USER-OPTIONAL flag in their Device Capabilities (TODO). If Client receives PAIRING-USER-OPTIONAL flag from a Device which also has SECURE-DEVICE flag - it MUST NOT allow using such a Device, with an appropriate report to the end-user.

If Client "pairs" with a Device which has PAIRING-USER-OPTIONAL set, it MAY ask user if he wants to perform "pairing". If PAIRING-USER-OPTIONAL flag is not set, Client MUST NOT allow to use Device (i.e. MUST NOT issue a program which resets MITM-CHECK-IN-PROGRESS Device flag, and MUST NOT send any non-pairing programs to the Device) until  "pairing" is actually performed. 

To re-iterate: being User-OPTIONAL means that while Device implementors still MUST implement MITM; however, under certain circumstances end-user MAY be allowed to skip MITM protection.

SINGLE-LED-PAIRING Built-In Plugin
##################################

TODO

References
----------

[Fortuna] Niels Ferguson, Bruce Schneier. "Practical Cryptography". Wiley Publishing, 2003. Sections 6.4 ('Fixing the Weaknesses')

