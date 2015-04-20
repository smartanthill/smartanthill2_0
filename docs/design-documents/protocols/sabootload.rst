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

.. _sabootload:

SmartAnthill Programming, Bootloaders and OtA Programming
=========================================================

:Version:   v0.1

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

In general, SmartAnthill supports several different deployment scenarios:

* SmartAnthill (no bootloader)
* SmartAnthill over 3rd-party-bootloader
* SmartAnthill-with-OtA-programming (no bootloader)
* SmartAnthill-with-OtA-programming over 3rd-party-bootloader

SmartAnthill and 3rd-party Bootloaders
--------------------------------------

In general, SmartAnthill implementation MAY run on top of a 3rd-party bootloader (for example, Zepto OS MAY be loaded over UART by 3rd-party bootloader), provided that 3rd-party bootloader complies to the following requirements:

* 3rd-party bootloader MUST require physical access to the SmartAnthill Device for the Device to be programmed

  + It means that **all** wireless/OtA 3rd-party bootloaders are **prohibited**
  + It also means that wired 3rd-party bootloaders are generally ok, as long as they comply with other requirements mentioned here

* 3rd-party bootloader MUST NOT allow extracting existing program from the SmartAnthill Device

SmartAnthill OtA Programming
----------------------------

SmartAnthill Devices which support SmartAnthill OtA Programming, MUST run SmartAnthill OtA Bootloader. Note that SmartAnthill OtA Bootloader MAY run either as a primary bootloader, or on top of 3rd-party bootloader (as long as 3rd-party bootloader satisfies requriements above). 

SmartAnthill Devices MAY support OtA reprogramming of the bootloader itself; however, in this case SmartAnthill Devices MUST comply with robustness requirements, and MUST ensure that if programming process is aborted for any reason, an old bootloader will continue to work. In particular, it means that when reprogramming a bootloader (i.e. OTA-TYPE=OTA_ROBUST_BOOTLOADER) and for "robust" OS programming (OTA-TYPE-OTA_ROBUST_OS):

* new bootloader MUST be loaded into an Flash/EEPROM area which is different from current bootloader
* atomicity requirements specified in OTA_COMMIT message below, MUST be complied with

Pairing with SmartAnthill OtA Programming
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Devices which support SmartAnthill OtA Programming, as any other SmartAnthill Device, perform "Pairing" (see :ref:`sapairing` document for details). As a result of pairing, SmartAnthill Device obtains a secret key which is shared with SmartAnthill Central Controller. 

For non-OtA-Programmable SmartAnthill Devices, this secret-key-obtained-from-pairing is used directly as a SASP key for normal (non-OtA-programming) communications. However, for OtA-Programmable SmartAnthill Devices, this secret-key-obtained-from-pairing is used as a OtA Programming key (and SASP key for normal communications MUST be an independent key, which passed as a part of the program passed over SmartAnthill-OtA-Programming-Protocol, which uses OtA Programming Key for authentication/encryption).

SmartAnthill OtA Programming Protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All OtA Programming of SmartAnthill Devices MUST be performed ONLY via the following SmartAnthill OtA Programming Protocol (SAOtAPP). SAOtAPP MUST be supported ONLY over SACCP over SAGDP over SASP, where SASP key MUST be OtA Programming Key (see above, obtained from "pairing"); OtA Programming Key MUST NOT be an all-zeros key (as defined in :ref:`sapairing`).

SAOtAPP consists of the following messages:

**\| OTA_CAPABILITIES_REQUEST \|**

where OTA_CAPABILITIES_REQUEST is a 1-byte constant. 

**\| OTA_CAPABILITIES_RESPONSE \| PLATFORM-ID \| FLAGS \| OS-ADDRESS \| BOOTLOADER-ADDRESS \| MAX-OS-SIZE \| MAX-BOOTLOADER-SIZE \| MAX-ROBUST-OS-SIZE \| MAX-RAM \| PLATFORM-SPECIFIC \|**

where OTA_CAPABILITIES_RESPONSE is a 1-byte constant, PLATFORM-ID is an Encoded-Unsigned-Int<max=2> field representing TODO enum, FLAGS is a 1-byte substrate for a bitfield, where bit [0] being OTA_OS_SUPPORT, bit [1] being OTA_ROBUST_BOOTLOADER_SUPPORT, bit [2] being OTA_ROBUST_OS_SUPPORT, and bits [3..7] reserved (MUST be zeros), PLATFORM-ADDRESS and BOOTLOADER-ADDRESS are Encoded-Unsigned-Int<max=2> fields representing addresses for which program and bootloader respectively should be compiled, MAX-OS-SIZE and MAX-BOOTLOADER-SIZE are Encoded-Unsigned-Int<max=2> fields specifying maximum possible sizes for os and bootloader, MAX-ROBUST-OS-SIZE is an Encoded-Unsigned-Int<max=2> field described below, MAX-RAM is an Encoded-Unsigned-Int<max=2> fields specifying maximum amount of RAM available on the Device, and PLATFORM-SPECIFIC fields depend on PLATFORM-ID (TODO). 

MAX-ROBUST-OS-SIZE field specifies maximum size of the os for which Device guarantees robust rewriting. If MAX-ROBUST-OS is zero, Device does not guarantee robust programming for OTA-TYPE=OTA_OS. Regardless of MAX-ROBUST-OS, Device MUST guarantee robustness for OTA-TYPE=OTA_ROBUST_BOOTLOADER (if it is supported).

*NB: to comply with "robustness" requirements, Device MAY need to return different addresses for OS-ADDRESS and/or BOOTLOADER-ADDRESS at different times; therefore, SmartAnthill Client MUST re-issue OTA_CAPABILITIES_REQUEST before every programming, and MUST NOT cache BOOTLOADER-ADDRESS and/or OS-ADDRESS*.

**\| OTA_START_REQUEST \| OTA-TYPE \| PROGRAM-ADDRESS \| PROGRAM-SIZE \| DATA-SIZE \| DATA \|**

where OTA_START is a 1-byte constant, OTA-TYPE is a 1-byte enum, which can be one of OTA_OS, OTA_ROBUST_OS, or OTA_ROBUST_BOOTLOADER, PROGRAM-ADDRESS is an Encoded-Unsigned-Int<max=2> field, which represents address for which the program (os or bootloader) has been compiled, PROGRAM-SIZE is a size of the whole program, DATA-SIZE is an Encoded-Unsigned-Int<max=2> field, and DATA has size of DATA-SIZE. 

OTA_START message instructs Device to start programming. If OTA-TYPE = OTA_OS, then previous OS MAY be discarded right away. However, if OTA-TYPE = OTA_ROBUST_*, existing OS/bootloader MUST be preserved intact until OTA_COMMIT message is received (and further processed as described in OTA_COMMIT message). If OTA_ROBUST_OS is requested but PROGRAM-SIZE > MAX-ROBUST-OS-SIZE returned in OTA_CAPABILITIES_RESPONSE, Device MAY return OTA_ERROR_TOOLARGE error.

OTA_START message starts a new OtA Programming Session. While OtA Programming Session is in progress, SACCP MUST block all the other messages and return TODO errors, until the session ends (either via OTA_ABORT_REQUEST or via OTA_COMMIT_REQUEST). Programming Session being in progress is specified by having OTA_PROGRAMMING_INPROGRESS in-RAM state.

If Device receives of any OTA messages except for OTA_CAPABILITIES_REQUEST and OTA_START_REQUEST when it is in OTA_PROGRAMMING_IDLE state - it is an OTA_ERROR_NOPROGRAMMING error.

**\| OTA_CONTINUE_REQUEST \| CURRENT-OFFSET \| DATA-SIZE \| DATA \|**

where OTA_CONTINUE is a 1-byte constant, CURRENT-OFFSET is an offset within the program (CURRENT-OFFSET is redundant, and MUST be equal to previous_OtA_message_offset + previous_OtA_message_data_size; otherwise it is a TODO error), and DATA-SIZE and DATA are similar to that of in OTA_START message. 

**\| OTA_ABORT_REQUEST \|**

where OTA_ABORT is a 1-byte constant. OTA_ABORT instructs Device to abort current programming session. The only valid reply to OTA_ABORT is OTA_ERROR with an error code OTA_ERROR_ABORTED.

**\| OTA_COMMIT_REQUEST \| CURRENT-OFFSET \| DATA-SIZE \| DATA \| PROGRAM-SIZE \| SACCP-CHECKSUM \|**

where OTA_COMMIT_REQUEST is a 1-byte constant, CURRENT-OFFSET, DATA-SIZE and DATA are similar to that of in OTA_CONTINUE_REQUEST message, PROGRAM-SIZE is overall program size (PROGRAM-SIZE is redundant, and MUST match PROGRAM-SIZE in OTA_START_REQUEST message, otherwise it is a TODO error), and SACCP-CHECKSUM is a SACCP checksum (as defined in :ref:`saccp` document) of the whole program.  

OTA_COMMIT_REQUEST message instructs the Device to check integrity of the program (using SACCP-CHECKSUM), and to "commit" current changes. In particular, for OTA-TYPE=OTA_ROBUST_BOOTLOADER and for OTA-TYPE=OTA_ROBUST_OS, Device MUST ensure atomic switch from existing bootloader to new (loaded) one. For example, it MAY be implemented as rewriting one single address within one single JMP instruction in the very beginning of the bootloader; it MUST NOT be implemented as copying of new bootloader to the old location (as it is not possible to ensure atomicity in this case, and bootloader might be lost).

**\| OTA_OK_RESPONSE \|** 

where OTA_OK_RESPONSE is a 1-byte constant. OTA_OK_RESPONSE can be sent in response to any of the following: OTA_START_REQUEST, OTA_CONTINUE_REQUEST, or OTA_COMMIT_REQUEST.

**\| OTA_ERROR_RESPONSE \| ERROR-CODE \|** 

where OTA_ERROR_RESPONSE is a 1-byte constant. OTA_ERROR_RESPONSE can be sent in response to any of the OTA_*_REQUEST messages. Error codes:  OTA_ERROR_ABORT, OTA_ERROR_TOOLARGE, OTA_ERROR_NOPROGRAMMING, the rest TODO.

All \*_REQUEST messages above are sent as a payload for SACCP OTA-REQUEST messages, and all \*_RESPONSE messages above are sent as a payload for SACCP OTA-RESPONSE messages.

*NB: Current implementation of SAOtAPP doesn't allow to use SAGDP Streaming (TODO). It means that it is slower than it might be; however, such decision simplifies and reduces portion of SmartAnthill Stack which needs to be implemented as a part of SmartAnthill OtA bootloader; TODO: study if adding streaming support makes sense*

TODO: OS referencing functions from BOOTLOADER

