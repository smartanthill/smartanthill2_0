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

.. _sapairing:

SmartAnthill Pairing
====================

:Version:   v0.0.10

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

"Pairing" SmartAnthill Device to SmartAnthill Client (which is normally located on SmartAnthill Central Controller) is necessary to ensure secure key exchange between SmartAnthill Device and SmartAnthill Client. As soon as "pairing" is completed, both parties have a 128-bit symmetric key shared between them, and can use it for SASP purposes.

SmartAnthill Pairing comes in several flavours. SmartAnthill Device MUST implement at least one of these flavours. SmartAnthill Client MUST implement *all* these flavours. 

SmartAnthill Pairing flavours are divided into two categories: Zero Pairing (which doesn't involve communication over SmartAnthill communication channel), and Over-the-Air (OtA) pairing. 

SmartAnthill Zero Pairing
-------------------------

Zero pairing doesn't involve communication over SmartAnthill communication channel.

SmartAnthill Zero Programming Pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SmartAnthill Zero Programming Pairing applies only to those devices which can be completely reprogrammed by SmartAnthill Central Controller (usually it applies to SmartAnthill Hobbyist Devices). It is a RECOMMENDED way of pairing for SmartAnthill Hobbyist Devices.

SmartAnthill Zero Programming Pairing consists of:

* Client generating secret key (see TODO for details)
* Client preparing (e.g. compiling or linking) a program which includes generated secret key as static data
* Client storing generated secret key in SA DB
* Client programming Device using prepared program (which contains generated secret key)

TODO: restrictions on SmartAnthill Device programming-socket key access.

SmartAnthill Zero Paper Pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SmartAnthill Zero Paper Pairing MAY be used by those SmartAnthill Mass-Market Devices, for which implementing other pairing methods is not feasible. **Zero Paper Pairing SHOULD NOT be used if other pairing methods are feasible. Zero Paper Pairing MUST NOT be used by Security Devices unless it is demonstrated that other pairing methods are not feasible for the Device.**

Zero Paper Pairing requires each Device to:

* have unique 128-bit crypto-random key programmed in as it's SASP AES key
* have this 128-bit key printed in the following user-friendly form:

  + 128-bit key is converted to a large unsigned integer (using SmartAnthill Endianness) from 0 to 2^128-1
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
      * Manufacturer MUST provide source code for the Device programming in a form which is used by SmartAnthill for programming of Hobbyist Devices

        + this source code MUST be available free of charge BOTH from manufacturer's web site, AND from an allowed third-party repository. List of allowed third-party source code repositories TBD (github, sourceforge, something else?)

    - key reprogramming. Protocols for key reprogramming over UART and over USB are TBD.

SmartAnthill OtA Pairing
------------------------

SmartAnthill OtA pairing provides security (including MITM protection) with minimal complexity involved.

From OtA Pairing perspective, SmartAnthill Device can be in one of the following OtA pairing states: 

* PRE-PAIRING
* PAIRING-ENTROPY-NEEDED (optional, see below)
* PAIRING-MITM-CHECK
* PAIRING-COMPLETED

IMPORTANT: Change from any of the states into PRE-PAIRING state MUST be implemented ONLY via physical manipulations of end-user with SmartAnthill Device (and NOT remotely). Examples of valid user interfaces to perform such a change include on-Device button or buttons (for example, if two buttons are simultaneously kept pressed for over N seconds) and on-Device PCB jumper.

In PRE-PAIRING and PAIRING-ENTROPY-NEEDED states, SASP MUST use 'zero' AES-128 key (with AES key consisting of all zeros). 

In PRE-PAIRING and PAIRING-ENTROPY-NEEDED states, no programs are allowed to be sent to SACCP; only TODO SACCP packets are allowed. In PAIRING-MITM-CHECK state, SACCP programs are allowed; however, in this state, SACCP restricts EXEC command of Zepto VM to the only Built-In bodypart (id=BUILTIN_BODYPART_PAIRING). 

From security perspective, SmartAnthill OtA pairing works as follows:

* BOTH parties generate 256-bit keys (see TODO for details on secure key generation), As an additional step for this particular key generation, if first 128 bits are all-zeros, key MUST be regenerated (this is a precaution against attacks on misimplementations).
* parties perform anonymous Diffie-Hellman key exchange for generated 256-bit keys, obtaining a 256-bit shared key
* from this point on, on both sides SASP starts to use first 128-bits of 256-bit shared key, as SASP AES key
* parties use last 128 bits of the 256-bit shared key ("MITM check key") to perform MITM protection check depending on the OtA pairing flavour. During this exchange, Device is kept in PAIRING-MITM-CHECK Device OtA pairing state. 
* if MITM protection check indicates that everything is fine - Device OtA pairing state is changed to PAIRING-COMPLETED, and normal work can be started.

Unique Devices and Hardware-Entropy-Based Devices
-------------------------------------------------

It should be understood that to ensure security, Devices MUST comply to at least one of the following two requirements:

* each device MUST be unique, i.e. MUST have some cryptographically random key (or keys) generated outside of Device and pre-programmed to the Device: 
  
  + Zero Pairing Devices MUST have unique pre-programmed key 
  + OtA Pairing Devices MUST have a cryptographic RNG, which MUST contain at least "Poor-Man's" PRNG, as described in TODO.

    - Poor-Man's PRNG is based on unique crypto-safe secret keys being pre-programmed to the Device

or

* Device MUST have cryptographic RNG with a hardware entropy source

  + this applies only to OtA Pairing Devices
  + Device MUST have a hardware entropy source, which provides a hardware-generated bit stream

  + if Poor-Man's PRNG was not pre-initialized out-of-Device:

    - such Devices are known as Hardware-Entropy-Only-Based Devices
    - Device MUST comply with requirements for Hardware-Entropy-Only-Based Devices as specified in :ref:`sarng`
    - Device MUST implement additional Pairing-Entropy-Needed - Pairing-Entropy-Provided exchange as described in *Specifics of OtA Pairing for Hardware-Entropy-Only-Based Devices* section below

  + if Poor-Man's PRNG was pre-initialized out-of-Device:

    - Device MUST comply with requirements for Unique-Hardware-Entropy-Based Devices, as specified in :ref:`sarng`
    - on-line testing MAY be omitted
    - Device still SHOULD implement additional Pairing-Entropy-Needed - Pairing-Entropy-Provided exchange as described in *Specifics of OtA Pairing for Hardware-Entropy-Only-Based Devices* section below

Ideally, for Devices with OtA Pairing, an RNG which combines both pre-initialized Poor-Man's PRNG and hardware cryptographic RNG, is RECOMMENDED. This combined approach SHOULD be used for SmartAnthill Security Devices. 

If only one of approaches is to be implemented, unique Device approach is generally RECOMMENDED over hardware-cryptographic approach. 

SmartAnthill OtA Pairing Protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pairing-Request: **\| OTA-PROTOCOL-VERSION-NUMBER \| DH-REQUEST \| ENTROPY \| CLIENT-OTA-AND-SASP-CAPABILITIES \|**

where OTA-PROTOCOL-VERSION-NUMBER is an Encoded-Unsigned-Int<max=2> field, DH-REQUEST is a 128-bit field, representing `g^a mod p` from DH key exchange (using SmartAnthill Endianness), ENTROPY is a 32-byte field with crypto-safe random data, and CLIENT-CAPABILITIES is TBD. ENTROPY is used for key generation as specified in :ref:`sarng` document; note that :ref:`sarng` requires 16 bytes of entropy per 128 bits of key, so to get 256 bits of key required for OtA Pairing, 32 bytes of ENTROPY is needed; first 16 bytes of ENTROPY are to be used to generate first 128 bits of key, and last 16 bytes of ENTROPY are to be used to generate last 256 bits of key. 

Pairing-Request is sent as a payload for a SACCP SACCP-OTA-PAIRING-REQUEST message. 

Pairing-Response: **\| DH-REPLY \| ACCEPTED-OTA-FLAVOUR \| DEVICE-SASP-CAPABILITIES \|**

where DH-REPLY is a 128-bit field, representing `g^b mod p` (using SmartAnthill Endianness), ACCEPTED-OTA-FLAVOUR is a 1-byte field containing an ID of accepted OtA flavour (TBD), and DEVICE-SASP-CAPABILITIES TBD.

Pairing-Response is sent as a payload for a SACCP SACCP-OTA-PAIRING-RESPONSE message. 

Instead of Pairing-Response, Device MAY (and in certain situations - MUST, see below) send an Pairing-Entropy-Needed-Response message (as a payload for a SACCP SACCP-OTA-PAIRING-ENTROPY-NEEDED-RESPONSE message):

Pairing-Entropy-Needed-Response: **\|** (empty body)

In response to Pairing-Entropy-Needed-Response, Client MUST reply with a Pairing-Entropy-Provided-Request (as a payload for a SACCP SACCP-OTA-PAIRING-ENTROPY-PROVIDED-REQUEST message)

Pairing-Entropy-Provided-Request: **\| ENTROPY \|**

where ENTROPY is a 16-byte field with cryptographically safe random data. 

When Device is satisfied with the amount of entropy it has, it should return a Pairing-Response to complete pairing. All the messages within one pairing procedure form a single "packet chain". That is, "packet chaing" may look as follows: Pairing-Request - Pairing-Entropy-Needed-Response - Pairing-Entropy-Provided-Request - [optionally more pairs of Entropy-Needed-Response and Entropy-Provided-Request] - Pairing-Response.

TODO: error message?

OtA Pairing Diffie-Hellman Paramaters
'''''''''''''''''''''''''''''''''''''

TODO

Specifics of OtA Pairing for Hardware-Entropy-Only-Based Devices
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Hardware-Entropy-Only-Based Devices MUST additionally implement the following procedure within OtA protocol: 

* During each Pairing, Device MUST return Pairing-Entropy-Needed-Response to the Client, at least 128 times; that is, "packet chain" which performs pairing, MUST look as follows: Pairing-Request - Pairing-Entropy-Needed-Response - Pairing-Entropy-Provided-Request - [at least 127 more pairs of Entropy-Needed-Response and Entropy-Provided-Request] - Pairing-Response

  + Between each Pairing-Entropy-Needed-Response and Pairing-Entropy-Provided-Request, Device MUST measure time interval with the best possible resolution. If possible, time interval SHOULD measured with down-to-single-clock-cycle precision; for example, unless other means of measuring time with single-clock-cycle precision are present, it is RECOMMENDED to implement waiting for packet (only for this very specific case) in a tight loop without any wait, counting iterations, with occasional checks if the data has arrived.
  + when receiving each of Pairing-Entropy-Provided-Request's, Device MUST feed both ENTROPY within request, and time interval measured, to the Fortuna PRNG (the same instance of Fortuna which is described in :ref:`sarng`). Form of such feeding is not essential, as long as all the bits both from ENTROPY field and from time interval measured, are fed to Fortuna.

OtA Pairing MITM-Check Program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After initial "packet chain" consisting of Pairing Request and Pairing Response, Device goes into PAIRING-MITM-CHECK state; MITM check is performed via "MITM-Check Program". 

MITM-Check Program is pretty much a regular Zepto VM program which goes over SACCP (over SAGDP over SASP). There is a difference from regular program though: MITM-Check Program MUST come only in PAIRING-MITM-CHECK Device pairing state. In this state, SACCP (and/or Zepto VM) prohibits program to access any bodyparts, except for a Built-In bodypart with id=BUILTIN_BODYPART_PAIRING. This also ensures that despite there can be two bodyparts accessing the same LED (one is 'pairing' bodypart, another is regular bodypart), there is no possible conflict between the two. 

OtA Pairing Flavours
^^^^^^^^^^^^^^^^^^^^

All OtA Pairing Flavours run on top of SmartAnthill OtA Pairing Protocol, and differ only in their MITM-Check Programs.

SmartAnthill OtA Single-LED Pairing
'''''''''''''''''''''''''''''''''''

SmartAnthill OtA Single-LED Pairing is pairing mechanism, which is automated (i.e. user is not required to enter any data), and which requires absolute minimum of resources on the Device side. Namely, all the Device needs to have (in addition to MCU) is one single LED. This LED MAY be any of existing LEDs on the Device. 

MITM-Check for Single-LED Pairing is performed as follows:

* User is asked to bring Device close to the webcam which is located on SmartAnthill Central Controller
* Client sends a MITM-Check program which requests LED to blink, using `Blinking-Function(random-nonce-sent-by-Client)=AES(key=MITM-check-key,data=random-nonce-sent-by-Client)` as a blinking pattern. TODO: Built-in Plugin to produce AES(...) reply.
* Accordingly, Device starts blinking the LED
* Client, using webcam, recognizes blinking pattern and makes sure that it matches expectations.
* If expectations don't match, program may be repeated with a *different* random-nonce-sent-by-Client
* If expectations do match, another program (also technically a MITM-Check program) is sent to change OtA Pairing State of the Device to PAIRING-COMPLETED.

NB: SmartAnthill Client SHOULD support using webcam on a smartphone camera for "pairing" purposes (provided that TODO requirements for securing communication between SmartAnthill Controller and smartphone's app, are met).

MITM-Check for Single-LED Pairing being User-OPTIONAL
#####################################################

All SmartAnthill Devices using Single-LED Pairing, MUST implement proper MITM Check procedures as described above. However, devices which are not designated as Security Devices, MAY set *both* LOW-SECURITY *and* PAIRING-USER-OPTIONAL flags in their Device Capabilities (TODO). If Client "pairs" with a Device which has *both* such flags set, it MAY ask user if he wants to perform "pairing". If at least one of the flags above is not set, Client MUST NOT allow to use Device (i.e. MUST NOT issue a program which resets MITM-CHECK-IN-PROGRESS Device flag, and MUST NOT send any non-pairing programs to the Device) until  "pairing" is actually performed. 

To re-iterate: being User-OPTIONAL means that while Device implementors still MUST implement MITM, in certain circumstances end-user MAY be allowed to skip MITM protection.

SINGLE-LED-PAIRING Built-In Plugin
##################################

TODO

SmartAnthill OtA Paper Pairing
''''''''''''''''''''''''''''''

TODO

