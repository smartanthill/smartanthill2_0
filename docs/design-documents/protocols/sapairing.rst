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

SmartAnthill PAIRING procedure
==============================

:Version:   v0.0.7

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

"Pairing" SmartAnthill Device to SmartAnthill Client (which is normally located on SmartAnthill Central Controller) is necessary to ensure secure key exchange between SmartAnthill Device and SmartAnthill Client. As soon as "pairing" is completed, both parties have a 128-bit symmetric key shared between them, and can use it for SASP purposes.

SmartAnthill Pairing comes in several flavours. SmartAnthill Device MUST implement at least one of these flavours. SmartAnthill Client MUST implement *all* these flavours. 

SmartAnthill Pairing flavours currently include SmartAnthill Zero Pairing, and SmartAnthill Single-LED Pairing.

SmartAnthill Zero Pairing
-------------------------

SmartAnthill Zero Pairing applies only to those devices which can be completely reprogrammed by SmartAnthill Central Controller (usually it applies to SmartAnthill Hobbyist Devices). 

SmartAnthill Zero Pairing consists of:

* Client generating secret key (see TODO for details)
* Client preparing (e.g. compiling or linking) a program which includes generated secret key as static data
* Client storing generated secret key in SA DB
* Client programming Device using prepared program (which contains generated secret key)

TODO: restrictions on SmartAnthill Device programming-socket key access.

SmartAnthill ESSP
-----------------

ESSP stands for 'Even Simpler Secure Pairing'. ESSP protocol provides security with minimal complexity involved.

From ESSP perspective, SmartAnthill Device can be in one of the following pairing states: 

* PRE-PAIRING
* PAIRING-KEY-EXCHANGE
* PAIRING-MITM-CHECK
* PAIRING-COMPLETED

Change from any of the states into PRE-PAIRING state MUST be implemented ONLY via physical manipulations of end-user with SmartAnthill Device (and NOT remotely). Examples of valid user interfaces to perform such a change include on-Device button (for example, if kept pressed for over N seconds) and on-Device PCB jumper.

In PRE-PAIRING and PAIRING-KEY-EXCHANGE states, SASP MUST use 'zero' AES-128 key (with AES key consisting of all zeros).

In PRE-PAIRING and PAIRING-KEY-EXCHANGE states, no programs are allowed to be sent to SACCP; only TODO SACCP packets are allowed. In PAIRING-MITM-CHECK state, SACCP programs are allowed; however, in this state, SACCP restricts EXEC command to the only Built-In bodypart (id=BUILTIN_BODYPART_PAIRING). 

From security perspective, ESSP works as follows:

* BOTH parties generate 256-bit keys (see TODO for details; prohibition on zero SASP key?)
* parties initiate anonymous Diffie-Hellman key exchange for generated 256-bit keys, obtaining a 256-bit shared key; this is reflected by PAIRING-KEY-EXCHANGE Device pairing state.
* from this point on, on both sides SASP starts to use first 128-bits of 256-bit shared key, as SASP AES key
* parties use last 128 bits of the 256-bit shared key ("MITM check key") to perform MITM protection check depending on the ESSP flavour. This is reflected by PAIRING-MITM-CHECK Device pairing state. 
* if MITM protection check indicates that everything is fine - Device state is changed to PAIRING-COMPLETED, and normal work can be started.

ESSP Protocol
^^^^^^^^^^^^^

**ESSP PAIRING REQUEST**

Format: TODO. Comes from Client to Device; when ESSP PAIRING REQUEST/RESPONSE is passed over SASP, SASP uses 'zero AES key' (TODO). Contains protocol version number, Client Capabilities, and `g^a mod p`

ESSP PAIRING REQUEST and ESSP PAIRING REPLY go over SACCP over SAGDP (over SASP), forming a 2-packet "packet chain", so that delivery is guaranteed by SAGDP.

**ESSP PAIRING REPLY**

Format: TODO. Comes from Device to Client; when ESSP PAIRING REQUEST/RESPONSE is passed over SASP, SASP uses 'zero AES key' (TODO). Contains Device Capabilities, and `g^b mod p`.
TODO: error message?

**ESSP PAIRING MITM PROGRAM**

ESSP PAIRING PROGRAM is not really a special message. Instead, it is a regular Zepto VM program which goes over SACCP over SAGDP over SASP. There is a difference from regular program though: this program MUST come only in PAIRING-MITM-CHECK Device pairing state. In this state, SACCP (and/or Zepto VM) prohibits program to access any bodyparts, except for a Built-In bodypart with id=BUILTIN_BODYPART_PAIRING. This also ensures that despite there can be two bodyparts accessing the same LED (one is 'pairing' bodypart, another is regular bodypart), there is no possible conflict between the two. 

ESSP DH Paramaters
''''''''''''''''''

TODO

SmartAnthill Single-LED Pairing
-------------------------------

SmartAnthill Single-LED Pairing is pairing mechanism, which requires absolute minimum of resources on the Device side. Namely, all the Device needs to have (in addition to MCU) is one single LED. This LED MAY be any of existing LEDs on the Device. 

SmartAnthill Single-LED Pairing is based on Even Simpler Secure Pairing (ESSP) protocol, as described above. MITM Check for Single-LED Pairing is performed as follows:

* User is asked to bring Device close to the webcam which is located on SmartAnthill Central Controller
* Client sends a program which asks LED to blink, using `Binking-Function(random-nonce-sent-by-Client)=AES(key=MITM-check-key,data=random-nonce-sent-by-Client)` as a pattern. TODO: Built-in Plugin to produce AES(...) reply.
* Accordingly, Device starts blinking the LED
* Client, using webcam, recognizes blinking pattern and makes sure that it matches expectations.
* If expectations don't match, program may be repeated with a *different* random-nonce-sent-by-Client

NB: optionally, a webcam located on a smartphone, can be used for this purpose (provided that TODO requirements for securing communication between SmartAnthill Controller and smartphone's app, are met).

Single-LED Pairing being User-OPTIONAL
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All SmartAnthill Devices MUST implement Single-LED Pairing. However, devices which are not designated as Security Devices, MAY set *both* LOW-SECURITY *and* PAIRING-USER-OPTIONAL flags in their Device Capabilities (TODO). If Client "pairs" with a Device which has *both* such flags set, it MAY ask user if he wants to perform "pairing". If at least one of the flags above is not set, Client MUST NOT allow to use Device (i.e. MUST NOT issue a program which resets MITM-CHECK-IN-PROGRESS Device flag, and MUST NOT send any non-pairing programs to the Device) until  "pairing" is actually performed. 

SINGLE-LED-PAIRING Built-In Plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO

