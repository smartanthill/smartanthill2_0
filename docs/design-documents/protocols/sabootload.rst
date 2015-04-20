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

:Version:   v0.1.3

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

SmartAnthill OtA Bootloader and SmartAnthill OS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SmartAnthill OtA Bootloader needs to implement and run most of SmartAnthill Protocol Stack - from SADLP-* to most of SACCP. Moreover, to enable re-programming over the same channel, SmartAnthill OtA Bootloader needs to run these parts of the stack even when OS is running on top of it. That's why, when a SmartAnthill OS (such as Zepto OS) is running on top of SmartAnthill OtA Bootloader, most of incoming packet processing is made within SmartAnthill OtA Bootloader, with only SACCP packets (provided that they're *neither* SACCP Pairing packets *nor* SACCP OtA Programming packets) fed to SmartAnthill OS (via "SACCP-OS Handler").

From the point of view of Zepto OS, when it needs to support OtA, it is simply split into two parts. The first part is SmartAnthill OtA Bootloader, which handles protocol layers starting from PHY and up to and including SAGDP; it also implements SACCP messages for Pairing and SAOtAPP protocols. All other SACCP messages are passed to the rest of Zepto OS, via "SACCP-OS Handler" implemented as `zepto_saccp_os_handler()` (TODO) function.

In addition, SmartAnthill OS part MAY be allowed to call certain functions (such as AES) of SmartAnthill OtA Bootloader (via "Dynamic Linking", described below); this MAY be used to reduce code footprint of the Zepto OS.

Pairing with SmartAnthill OtA Programming
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Devices which support SmartAnthill OtA Programming, as any other SmartAnthill Device, perform "Pairing" (see :ref:`sapairing` document for details). As a result of pairing, SmartAnthill Device obtains a secret key which is shared with SmartAnthill Central Controller. 

For non-OtA-Programmable SmartAnthill Devices, this secret-key-obtained-from-pairing is used directly as a SASP key for SmartAnthill OS communications. However, for OtA-Programmable SmartAnthill Devices, this secret-key-obtained-from-pairing is used as a OtA Programming key (and SASP key for SmartAnthill OS communications MUST be an independent key, which passed as a part of SmartAnthill-OtA-Programming-Protocol echange, which uses OtA Programming Key for authentication/encryption). 

NB: if necessary, OtA programming MAY be used to change SmartAnthill OS SASP key (via reloading the whole SmartAnthill OS); this MAY be used for several reasons, including refreshing keys and dealing with running-out-of-SASP-nonces scenarios. In addition, OTA_BOOTLOADER mode of SAOtAPP MAY be used to refresh OTA Programming Key itself.

SmartAnthill OtA Programming Protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All OtA Programming of SmartAnthill Devices MUST be performed ONLY via the following SmartAnthill OtA Programming Protocol (SAOtAPP). SAOtAPP MUST be supported ONLY over SACCP over SAGDP over SASP, where SASP key MUST be OtA Programming Key (see above, obtained from "pairing"); OtA Programming Key MUST NOT be an all-zeros key (as defined in :ref:`sapairing`).

SAOtAPP consists of the following messages:

OtA Capabilities Request: **\|** (empty body). *"Additional SACCP Bits": 0x0*

OtA Capabilities Response: **\| PLATFORM-ID \| FLAGS \| OS-ADDRESS \| BOOTLOADER-ADDRESS \| MAX-OS-SIZE \| MAX-BOOTLOADER-SIZE \| MAX-ROBUST-OS-SIZE \| MAX-RAM \| PLATFORM-SPECIFIC \|** *"Additional SACCP Bits": 0x0*

where PLATFORM-ID is an Encoded-Unsigned-Int<max=2> field representing TODO enum, FLAGS is a 1-byte substrate for a bitfield, where bit [0] being OTA_OS_SUPPORT, bit [1] being OTA_ROBUST_BOOTLOADER_SUPPORT, bit [2] being OTA_ROBUST_OS_SUPPORT, bit [3] being OTA_DYNAMIC_LINKING_SUPPORT, and bits [4..7] reserved (MUST be zeros), PLATFORM-ADDRESS and BOOTLOADER-ADDRESS are Encoded-Unsigned-Int<max=2> fields representing addresses for which program and bootloader respectively should be compiled, MAX-OS-SIZE and MAX-BOOTLOADER-SIZE are Encoded-Unsigned-Int<max=2> fields specifying maximum possible sizes for os and bootloader, MAX-ROBUST-OS-SIZE is an Encoded-Unsigned-Int<max=2> field described below, MAX-RAM is an Encoded-Unsigned-Int<max=2> fields specifying maximum amount of RAM available on the Device, and PLATFORM-SPECIFIC fields depend on PLATFORM-ID (TODO). 

MAX-ROBUST-OS-SIZE field specifies maximum size of the os for which Device guarantees robust rewriting. If MAX-ROBUST-OS is zero, Device does not guarantee robust programming for OTA-TYPE=OTA_OS. Regardless of MAX-ROBUST-OS, Device MUST guarantee robustness for OTA-TYPE=OTA_ROBUST_BOOTLOADER (if it is supported).

*NB: to comply with "robustness" requirements, Device MAY need to return different addresses for OS-ADDRESS and/or BOOTLOADER-ADDRESS at different times; therefore, SmartAnthill Client MUST re-issue OtA Capabilities Request before every programming, and MUST NOT cache BOOTLOADER-ADDRESS and/or OS-ADDRESS*.

OtA Dynamic Linking Request: **\|** (empty body). *"Additional SACCP Bits": 0x1*

OtA Dynamic Linking Response: **\ BOOTLOADER-VENDOR \| BOOTLOADER-VERSION \| BOOTLOADER-COMPILER-ID \| BOOTLOADER-COMPILER-VERSION \| BOOTLOADER-COMPILER-CALLING-CONVENTION-ID \| DYNAMIC-LINK-ID1 \| DYNAMIC-LINK-ADDRESS1 \| DYNAMIC-LINK-ID2 \| DYNAMIC-LINK-ADDRESS2 | ... \|** *"Additional SACCP Bits": 0x1*

where BOOTLOADER-VENDOR (TODO: list and way to apply for one), BOOTLOADER-VERSION (up to vendor, but SHOULD be monotonous), and BOOTLOADER-COMPILER-ID (TODO: list) are Encoded-Unsigned-Int<max=2> fields, BOOTLOADER-COMPILER-VERSION is a null-terminated string such as "4.8.1a", BOOTLOADER-COMPILER-CALLING-CONVENTION-ID (TODO:list) is an Encoded-Unsigned-Int<max=2> field, DYNAMIC-LINK-ID\* is an Encoded-Unsigned-Int<max=2> field, specifying dynamic-id, and DYNAMIC-LINK-ADDRESS\* field is an absolute address of the corresponding function (residing within current Bootloader). Only implemented functions are listed in OtA Dynamic Linking Response.

Dynamic linking is a mechanism which allows SmartAnthill OS to call certain functions from SmartAnthill Bootloader to save on size; it is described in more detail below.

OtA Start Request: **\| OTA-TYPE \| PROGRAM-ADDRESS \| PROGRAM-ENTRY-POINT \| OPTIONAL-KEY \| PROGRAM-SIZE \| DATA-SIZE \| DATA \|** *"Additional SACCP Bits": 0x2*

where OTA-TYPE is a 1-byte enum, which can be one of OTA_OS, OTA_ROBUST_OS, or OTA_ROBUST_BOOTLOADER, PROGRAM-ADDRESS is an Encoded-Unsigned-Int<max=2> field, which represents address for which the program (os or bootloader) has been compiled, PROGRAM-ENTRY-POINT is a point where control should be passed within PROGRAM (for OTA-TYPE=OTA_*OS, it is an address of "SACCP-OS Handler" as described above, for OTA-TYPE=OTA_ROBUST_BOOTLOADER it is usually the same as PROGRAM-ADDRESS), OPTIONAL-KEY is a 16-byte field, which is present only if OTA-TYPE=OTA_*OS, and represents SmartAnthill OS SASP key, PROGRAM-SIZE is a size of the whole program, DATA-SIZE is an Encoded-Unsigned-Int<max=2> field, and DATA has size of DATA-SIZE. 

OtA Start Request message instructs Device to start programming. If OTA-TYPE = OTA_OS, then previous OS MAY be discarded right away. However, if OTA-TYPE = OTA_ROBUST_*, existing OS/bootloader MUST be preserved intact until OtA Commit message is received (and further processed as described in OtA Commit message). If OTA_ROBUST_OS is requested but PROGRAM-SIZE > MAX-ROBUST-OS-SIZE returned in OTA_CAPABILITIES_RESPONSE, Device MAY return OTA_ERROR_TOOLARGE error.

OtA Start Request message starts a new OtA Programming Session. While OtA Programming Session is in progress, SACCP MUST block all the other messages and return TODO errors, until the session ends (either via OtA Abort Request or via OtA Commit Request). Programming Session being in progress is specified by having OTA_PROGRAMMING_INPROGRESS in-RAM state.

If Device receives of any OTA messages except for OtA Capabilities Request, OtA Dynamic Linking Request, and OtA Start Request when it is in OTA_PROGRAMMING_IDLE state - it is an OTA_ERROR_NOPROGRAMMING error.

OtA Continue Request: **\| CURRENT-OFFSET \| DATA-SIZE \| DATA \|** *"Additional SACCP Bits": 0x3*

where CURRENT-OFFSET is an offset within the program (CURRENT-OFFSET is redundant, and MUST be equal to previous_OtA_message_offset + previous_OtA_message_data_size; otherwise it is a TODO error), and DATA-SIZE and DATA are similar to that of in OtA Start message. 

OtA Abort Request: **\|** (empty body) *"Additional SACCP Bits": 0x4*

OtA Abort Request instructs Device to abort current programming session. The only valid reply to OtA Abort Request is OtA Error Response with an error code OTA_ERROR_ABORTED.

OtA Commit Request: **\| CURRENT-OFFSET \| DATA-SIZE \| DATA \| PROGRAM-SIZE \| SACCP-CHECKSUM \|** *"Additional SACCP Bits": 0x5*

where CURRENT-OFFSET, DATA-SIZE and DATA are similar to that of in OtA Continue Request message, PROGRAM-SIZE is overall program size (PROGRAM-SIZE is redundant, and MUST match PROGRAM-SIZE in OtA Start Request message, otherwise it is a TODO error), and SACCP-CHECKSUM is a SACCP checksum (as defined in :ref:`saccp` document) of the whole program.  

OtA Commit Request message instructs the Device to check integrity of the program (using SACCP-CHECKSUM), and to "commit" current changes. In particular, for OTA-TYPE=OTA_ROBUST_BOOTLOADER and for OTA-TYPE=OTA_ROBUST_OS, Device MUST ensure atomic switch from existing bootloader to new (loaded) one. For example, it MAY be implemented as rewriting one single address within one single JMP instruction in the very beginning of the bootloader; it MUST NOT be implemented as copying of new bootloader to the old location (as it is not possible to ensure atomicity in this case, and bootloader might be lost).

OtA Ok Response: **\|** (empty body) *"Additional SACCP Bits": 0x2*

OtA Ok Response can be sent in response to any of the following: OtA Start Request, OtA Continue Request, or OtA Commit Request.

OtA Error Response: **\| ERROR-CODE \|** *"Additional SACCP Bits": 0x3*

where ERROR-CODE is an Encoded-Unsigned-Int<max=2> field. OtA Error Response MAY be sent in response to any of the OtA \* Request messages. Error codes:  OTA_ERROR_ABORT, OTA_ERROR_TOOLARGE, OTA_ERROR_NOPROGRAMMING, the rest TODO.

All OtA \* Request messages above are sent as a payload for SACCP OTA-REQUEST messages (with "Additional SACCP Bits" passed alongside), and all OtA \* Response messages above are sent as a payload for SACCP OTA-RESPONSE messages (with "Additional SACCP bits" passed alongside).

*NB: Current implementation of SAOtAPP doesn't allow to use SAGDP Streaming (TODO). It means that it is slower than it might be; however, such decision simplifies and reduces portion of SmartAnthill Stack which needs to be implemented as a part of SmartAnthill OtA bootloader; TODO: study if adding streaming support makes sense*

Dynamic Linking
^^^^^^^^^^^^^^^

To allow saving on size of large (by MCU standards) functions such as AES and EAX, SmartAnthill Device MAY support a "Dynamic Linking" mechanism. In this case, Device SHOULD return OTA_DYNAMIC_LINKING_SUPPORT flag in OtA Capabilities response, and SHOULD return a list of implemented functions in OtA Dynamic Linking response. Each supported function has it's own well-known ID; each ID specifies not only a function name, but an exact C prototype, so when prototype changes, it requires introducing new ID. 

Currently supported functions include:

+--------------------------------------+----------------------------------------------------------------------+
| ID                                   | Prototype                                                            |
+======================================+======================================================================+
| DYNAMIC_LINK_AES_ENCRYPT             | `void aes_encrypt(void* block, const void* key);` TODO               |
+--------------------------------------+----------------------------------------------------------------------+
| DYNAMIC_LINK_EAX_ENCRYPTAUTH         | TODO                                                                 |
+--------------------------------------+----------------------------------------------------------------------+
| DYNAMIC_LINK_EAX_DECRYPTAUTHCHECK    | TODO                                                                 |
+--------------------------------------+----------------------------------------------------------------------+
| DYNAMIC_LINK_SACCP_CHECKSUM          | TODO                                                                 |
+--------------------------------------+----------------------------------------------------------------------+

TODO: more if applicable

