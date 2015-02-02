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

.. _saprotostack:

SmartAnthill 2.0 Protocol Stack
===============================

:Version:   v0.2

*NB: this document relies on certain terms and concepts introduced in*
:ref:`saoverarch` *document, please make sure to read it before proceeding.*

SmartAnthill protocol stack is intended to provide communication services between SmartAnthill Clients and SmartAnthill Devices, allowing SmartAnthill Clients to control SmartAnthill Devices. These communication services are implemented as a request-response services within OSI/ISO network model.

.. contents::


Actors
------

In SmartAnthill architecture, there are three distinct entities:

* **SmartAnthill Client**. Whoever needs to control SmartAnthill Device(s). SmartAnthill Clients are usually implemented by SmartAnthill Central Controllers (which normally reside on the same physical machines as SmartAnthill Routers), but this is not strictly required, and is not the case in some topologies (such as distributed SmartAnthill network as shown on TODO).
* **SmartAnthill Router**. SmartAnthill Router allows to control SmartAnthill Devices connected to it. Performs conversion between SAoIP and SADLP-* protocols (see below).
* **SmartAnthill Device**. Physical device (containing sensor(s) and/or actuator(s)), which implements at least some parts of SmartAnthill protocol stack. SmartAnthill Devices can be further separated into:

  + **SmartAnthill Simple Device**. SmartAnthill Device which is not powerful enough to run it's own SAoIP/IP stack. MUST be connected via SmartAnthill Router.
  + **SmartAnthill IP-enabled Device**. SmartAnthill Device which is enabled to run it's own SAoIP/IP stack. MAY be connected without SmartAnthill Router directly to Intranet/Internet. Note that implementing TCP is not strictly required for SmartAnthill IP-enabled Devices.

Addressing
----------

In SmartAnthill, each SmartAnthill Device is assigned it's own address, which is a pair (IPv6 address,port number). It allows to have either one IP address per device, or to have multiple devices per IP address as necessary and/or convenient.

Relation between SmartAnthill protocol stack and OSI/ISO network model
----------------------------------------------------------------------

+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| Layer  | OSI-Model    | SmartAnthill     |     Function          | Implementation       | Implementation         | Implementation             | Implementation         |
|        |              | Protocol Stack   |                       | on Clients           | on IP-enabled Devices  | on Routers                 | on Simple Devices      |
|        |              |                  |                       |                      |                        +---------------+------------+                        |
|        |              |                  |                       |                      |                        | IP side       | SA side    |                        |
+========+==============+==================+=======================+======================+========================+===============+============+========================+
| 7      | Application  | SACCP            | Device Control        | Control Program      | Yocto VM               | --                         | Yocto VM               |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| 6      | Presentation | --               |                       |                      |                        |                            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| 5      | Session      | SAGDP            | Guaranteed            | SAGDP ("Master")     | SAGDP ("Slave")        | --                         | SAGDP ("Slave")        |
|        |              |                  | Delivery              |                      |                        |                            |                        |
|        |              +------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
|        |              | SASP             | Encryption and        | SASP                 | SASP                   | SASP (optional)            | SASP                   |
|        |              |                  | Authentication        |                      |                        |                            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+----------------------------+------------------------+
| 4      | Transport    | SAoIP [1]_       | Transport over IP     | SAoIP                | SAoIP                  | SAoIP                      | --                     |
|        |              |                  | Networks              |                      |                        |                            |                        |
|        |              +------------------+-----------------------+----------------------+------------------------+---------------+------------+                        |
|        |              | TCP or UDP       | As usual for TCP/UDP  | TCP or UDP           | TCP or UDP             | TCP or UDP    | --         |                        |
|        |              |                  |                       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+            |                        |
| 3      | Network      | IP               | As usual for IP       | IP                   | IP                     | IP            |            |                        |
|        |              |                  |                       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+
| 2      | Datalink     | SADLP-*          | Intra-bus addressing, | -- (standard network | -- (standard network   | -- (std netwk | SADLP-*    | SADLP-*                |
|        |              |                  | Fragmentation         | capabilities)        | capabilities)          | capabilities) |            |                        |
|        |              |                  | (if applicable)       |                      |                        |               |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+
| 1      | Physical     | Physical         |                       | -- (standard network | -- (standard network   | -- (std netwk | Physical   | Physical               |
|        |              |                  |                       | capabilities)        | capabilities)          | capabilities) |            |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+---------------+------------+------------------------+

.. [1] For Simple SmartAnthill Devices, SAoIP is translated into SADLP-* by the SmartAnthill Router (the one which directly controls the SmartAnthill Device). It can (and SHOULD) be done in a completely transparent manner, so that SmartAnthill Client SHOULD be able to communicate with SmartAnthill Device in exactly the same manner regardless of SmartAnthill Device being IP-enabled Device or Simple Device.

SmartAnthill protocol stack consists of the following protocols:

* **SACCP** – SmartAnthill Command&Control Protocol. Corresponds to Layer 7 of OSI/ISO network model. On the SmartAnthill Device side, is implemented by Yocto VM, which handles generic commands and routes device-specific commands to device-specific plug-ins.

* **SAGDP** – SmartAnthill Guaranteed Delivery Protocol. Belongs to Layer 5 of OSI/ISO network model. Provides guaranteed command/reply delivery. Flow control is implemented, but is quite rudimentary (only one outstanding packet is normally allowed for each virtual link, see details below). On the other hand, SAGDP provides efficient support for scenarios such as temporary disabling receiver on the SmartAnthill Device side; such scenarios are very important to ensure energy efficiency.

* **SASP** – SmartAnthill Security Protocol. Due to several considerations (including resource constraints) SmartAnthill protocol stack implements security on a layer right below SAGDP, so SASP essentially belongs to Layer 5 of OSI/ISO network model.

* **SAoIP** – SmartAnthill over IP Protocol. Lies right on top of TLS, TCP or UDP. SAoIP is not implemented on SmartAnthill Simple Devices, and all the SAoIP headers are stripped by SmartAnthill Router before passing the data to SmartAnthill Simple Device.

* **SADLP-\*** – SmartAnthill DataLink Protocol family. Belongs to Layer 2 of OSI/ISO network model. SADLP-* is specific to an underlying transfer technology (so for CAN bus SADLP-CAN is used, for IEEE 802.15.4 SADLP-IEEE802.15.4 is used). SADLP-* handles fragmentation if necessary and provides non-guaranteed packet transfer.


Error Handling Philosophy and Asymmetric Nature
-----------------------------------------------
In real-world operation, it is inevitable that from time to time a mismatch occurs between the states of SmartAnthill Central Controller and SmartAnthill Device; while such mismatches should never occur as long as the SmartAnthill protocols are strictly adhered to, mistmatches still may occur for many practical reasons, such as reboot or restore-from-backup of SmartAnthill Central Controller, a transient failure of the SmartAnthill Device (for example, due to power surge, near-depleted battery, RAM soft error due to cosmic rays, etc.).

SmartAnthill protocol stack attempts to clear as many such scenarios as possible 'automagically', without the need to reprogram SmartAnthill Device. To achieve this goal, the following approach is used: SmartAnthill protocol stack assumes that in any case when there is any kind of the mismatch, it is the SmartAnthill Central Controller who's "right". In addition, if such a decision is not sufficient to recover from the mismatch, SmartAnthill Device will perform complete re-initialization.

It means that certain SmartAnthill protocols (such as SACCP and SAGDP) are inherently asymmetrical; details are provided in their respective documents (
:ref:`saccp`  and 
:ref:`sagdp` ).

TODO: recommend on-device self-recovery circuit?


Packet Chains
-------------

SmartAnthill protocol stack is intended to provide various services between two entities: SmartAnthill Central Controller and SmartAnthill Device. Most of these services are of request-response nature. To implement them while imposing the least requirements on the resource-stricken SmartAnthill Device, all interactions within SmartAnthill protocol stack at the levels between SACCP and SAGDP (inclusive) are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on.

Chains are initiated by the topmost protocol is SmartAnthill protocol layer, SACCP, and are supported by all the layers between SACCP and SAGDP (inclusive). Whenever SACCP issues a packet to an underlying protocol, it MUST specify whether a packet is a first, intermediate, or last within a “packet chain” (using 'is-first' and 'is-last' flags; note that due to “rules of engagement” described below, 'is-first' and 'is-last' flags are inherently incompatible, which MAY be relied on by implementation). This information allows underlying protocols (down to SAGDP) to arrange for proper retransmission if some packets are lost during communication, see 
:ref:`sagdp` document for details.

Starting from OSI Layer 2 and above, there is a virtual link established between SmartAnthill Central Controller and SmartAnthill Device. Normally (as guaranteed by SAGDP) only one outstanding packet is allowed on each such virtual link. There is one exception to this rule, which is described below.

Handling of temporary dual “packet chains”
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Normally, at each moment for each of the 'virtual links' decribed above, there can be only one “packet chain” active, and within a “packet chain”, all transmissions are always sequential. However, there are scenarios when both SmartAnthill Central Controller and SmartAnthill Device try to initiate their own “packet chains”. One such example is when SmartAnthill Device is sleeping according to instructions received from SmartAnthill Central Controller (and just woke up to perform task and report), and meanwhile SmartAnthill Central Controller has made a decision (for example, due to the input from other SmartAnthill Devices or from the end-user) to issue different set of instructions to the SmartAnthill Device.

Handling of these scenarios is explained in detail in respective documents (
:ref:`saccp` and 
:ref:`sagdp` ); as a result of such handling, one of the chains (the one coming from the SmartAnthill Device, according to "Central Controller is always right" principle described above), will be dropped pretty much as if it has never been started.


Layering remarks
----------------

SACCP and "packet chains"
^^^^^^^^^^^^^^^^^^^^^^^^^

SACCP is somewhat unusual for an application-level protocol in a sense that SACCP needs to have some knowledge about "packet chains" which are implicitly related to retransmission correctness. This is a conscious design choice of SACCP (and SAGDP) which has been made in face of extremely constrained (and unusual for conventional communication) environments which SmartAnthill protocol stack needs to support. It should also be noted that while some such details are indeed exposed to SACCP, they are formalized as a clear set of “rules of engagement” to be obeyed. As long as these “rules of engagement” are complied with, SACCP does not need to care about retransmission correctness (though the rationale for “rules of engagement” is still provided by retransmission correctness).

SASP below SAGDP
^^^^^^^^^^^^^^^^

It is somewhat unusual to have encryption layer (SASP) "below" transport/session layer (SAGDP). This is a conscious design choice of SASP/SAGDP. In particular, it allows to:

* rely that all the packets reaching SAGDP layer, are already authenticated; this allows (at the cost of the authenticating potentially malicious packets) to:

  + avoid attacks such as sending RST to disrupt logical connection
  + avoid attacks similar to "SYN flood" attacks

* allows to implement "Trusted Router" nodes in a simple manner (without implementing SAGDP on the router).

