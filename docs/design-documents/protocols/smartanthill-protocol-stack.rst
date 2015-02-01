v0.1.1

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

SmartAnthill Protocol Stack
===========================

*NB: this document relies on certain terms and concepts introduced in “SmartAnthill Overall Architecture” document, please make sure to read it before proceeding.*

SmartAnthill protocol stack is intended to provide communication services between SmartAnthill Control Centers and SmartAnthill Devices, allowing SmartAnthill Control Centers to control SmartAnthill Devices. These communication services are implemented as a request-response services within OSI/ISO network model.

.. contents::

Relation between SmartAnthill protocol stack and OSI/ISO network model
----------------------------------------------------------------------

+--------+--------------+------------------+-----------------------+----------------------+------------------------+
| Layer  | OSI-Model    | SmartAnthill     |     Function          | Implementation       | Implementation         |
|        |              | Protocol Stack   |                       | on Control Center(s) | on Devices             |
+========+==============+==================+=======================+======================+========================+
| 7      | Application  | SACP             | Device Control        | Control Program      | Yocto VM               |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+
| 6      | Presentation | --               |                       |                      |                        |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+
| 5      | Session      | SAMP [1]_        | Support for           | SAMP                 |   --                   |
+--------+--------------+                  | Multi-Node Topologies |                      |                        | 
| 4      | Transport    |                  |                       |                      |                        |
+--------+--------------+                  |                       |                      |                        |
| 3      | Network      |                  |                       |                      |                        |
|        |              |                  |                       |                      |                        |
+--------+----------+---+------------------+-----------------------+----------------------+------------------------+
|        |          |LLC| SAGDP            | Guaranteed            | SAGDP ("Master")     | SAGDP ("Slave")        |
|        |          |   |                  | Delivery              |                      |                        |
|        |          |   +------------------+-----------------------+----------------------+------------------------+
|        |          |   | SASP             | Datalink Encryption   | SASP                 | SASP                   |
|        |          |   |                  | and Authentication    |                      |                        |
| 2      | Data Link+---+------------------+-----------------------+----------------------+------------------------+
|        |          |MAC| SADLP-*          | Intra-bus addressing, | SADLP-*              | SADLP-*                |
|        |          |   |                  | Fragmentation         |                      |                        |
|        |          |   |                  | (if applicable)       |                      |                        |
+--------+----------+---+------------------+-----------------------+----------------------+------------------------+
| 1      | Physical     | Physical         |                       | Physical             | Physical               |
+--------+--------------+------------------+-----------------------+----------------------+------------------------+

.. [1] See note in "Layering Remarks" -> "SAMP and SACP" section below

SmartAnthill protocol stack consists of the following protocols:

* **SACP** – SmartAnthill Control Protocol. Corresponds to Layer 7 of OSI/ISO network model. On the SmartAnthill Device side, is implemented by Yocto VM, which handles generic commands and routes device-specific commands to device-specific plug-ins.

* **SAMP** – SmartAnthill Meta Protocol. Roughly covers Layers 3-5 of OSI/ISO network model. SAMP is not implemented on SmartAnthill Devices, and all the SAMP headers are stripped by the last SmartAnthill Control Center before passing the data to SmartAnthill Device. It means that SAMP is pretty much optional unless multi-node SmartAnthill Control Center topologies (TODO: add to "SmartAnthill Overall Architecture") are used. 

* **SAGDP** – SmartAnthill Guaranteed Delivery Protocol. Belongs to Logical Control Layer (LLC) of Data Link Layer (Layer 2) of OSI/ISO network model. Provides guaranteed command/reply delivery. Flow control is implemented, but is quite rudimentary (only one outstanding packet is normally allowed for each virtual link, see details below). On the other hand, SAGDP provides efficient support for scenarios such as temporary disabling receiver on the SmartAnthill Device side; such scenarios are very important to ensure energy efficiency.

* **SASP** – SmartAnthill Security Protocol. Due to several considerations (including resource constraints) SmartAnthill protocol stack implements security as a part of Logical Control Layer (LLC) of Data Link layer (Layer 2), so SASP essentially belongs to Layer 2 of OSI/ISO network model. 

* **SADLP-\*** – SmartAnthill DataLink Protocol family. Belongs to Layer 2 of OSI/ISO network model (right below SASP). SADLP-* is specific to an underlying transfer technology (so for CAN bus SADLP-CAN is used, for Zigbee SADLP-ZigBee is used). SADLP-* handles fragmentation if necessary and provides non-guaranteed packet transfer. 


Error Handling Philosophy and Asymmetric Nature
-----------------------------------------------
In real-world operation, it is inevitable that from time to time a mismatch occurs between the states of SmartAnthill Central Controller and SmartAnthill Device; while such mismatches should never occur as long as the SmartAnthill protocols are strictly adhered to, mistmatches still may occur for many practical reasons, such as reboot or restore-from-backup of SmartAnthill Central Controller, a transient failure of the SmartAnthill Device (for example, due to power surge, near-depleted battery, RAM soft error due to cosmic rays, etc.). 

SmartAnthill protocol stack attempts to clear as many such scenarios as possible 'automagically', without the need to reprogram SmartAnthill Device. To achieve this goal, the following approach is used: SmartAnthill protocol stack assumes that in any case when there is any kind of the mismatch, it is the SmartAnthill Central Controller who's "right". In addition, if such a decision is not sufficient to recover from the mismatch, SmartAnthill Device will perform complete re-initialization. 

It means that certain SmartAnthill protocols (such as SACP and SAGDP) are inherently asymmetrical; details are provided in their respective documents ("SACP" and "SAGDP").

TODO: recommend on-device self-recovery circuit?


Packet Chains
-------------

SmartAnthill protocol stack is intended to provide various services between two entities: SmartAnthill Central Controller and SmartAnthill Device. Most of these services are of request-response nature. To implement them while imposing the least requirements on the resource-stricken SmartAnthill Device, all interactions within SmartAnthill protocol stack at the levels between SACP and SAGDP (inclusive) are considered as “packet chains”, when one of the parties initiates communication by sending a packet P1, another party responds with a packet P2, then first party may respond to P2 with P3 and so on. 

Chains are initiated by the topmost protocol is SmartAnthill protocol layer, SACP, and are supported by all the layers between SACP and SAGDP (inclusive). Whenever SACP issues a packet to an underlying protocol, it MUST specify whether a packet is a first, intermediate, or last within a “packet chain” (using 'is-first' and 'is-last' flags; note that due to “rules of engagement” described below, 'is-first' and 'is-last' flags are inherently incompatible, which MAY be relied on by implementation). This information allows underlying protocols (down to SAGDP) to arrange for proper retransmission if some packets are lost during communication, see "SAGDP" document for details.

Starting from SADLP-* and above, there is a virtual link established between SmartAnthill Central Controller and SmartAnthill Device. Normally (as guaranteed by SAGDP) only one outstanding packet is allowed on each such virtual link. There is one exception to this rule, which is described below.

Handling of temporary dual “packet chains”
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Normally, at each moment for each of the 'virtual links' decribed above, there can be only one “packet chain” active, and within a “packet chain”, all transmissions are always sequential. However, there are scenarios when both SmartAnthill Central Controller and SmartAnthill Device try to initiate their own “packet chains”. One such example is when SmartAnthill Device is sleeping according to instructions received from SmartAnthill Central Controller (and just woke up to perform task and report), and meanwhile SmartAnthill Central Controller has made a decision (for example, due to the input from other SmartAnthill Devices or from the end-user) to issue different set of instructions to the SmartAnthill Device.

Handling of these scenarios is explained in detail in respective documents ("SACP" and "SAGDP"); as a result of such handling, one of the chains (the one coming from the SmartAnthill Device, according to "Central Controller is always right" principle described above), will be dropped pretty much as if it has never been started.


Layering remarks
----------------

SACP and "packet chains"
^^^^^^^^^^^^^^^^^^^^^^^^

SACP is somewhat unusual for an application-level protocol in a sense that SACP needs to have some knowledge about "packet chains" which are implicitly related to retransmission correctness. This is a conscious design choice of SACP (and SAGDP) which has been made in face of extremely constrained (and unusual for conventional communication) environments which SmartAnthill protocol stack needs to support. It should also be noted that while some such details are indeed exposed to SACP, they are formalized as a clear set of “rules of engagement” to be obeyed. As long as these “rules of engagement” are complied with, SACP does not need to care about retransmission correctness (though the rationale for “rules of engagement” is still provided by retransmission correctness). 

SAMP and SACP
^^^^^^^^^^^^^

From a certain perspective, SAMP is a higher-level protocol than SACP - it is SAMP which encapsulates SACP, not vice versa. However, SAMP implements services which are typical for the OSI/ISO Layers 3-5, so in the table above we've listed it accordingly. 

In general, it is not worth arguing where exactly certain protocol belongs in the OSI/ISO model, and we provide the table above only to help  with understanding of SmartAnthill protocol stack, not to argue "what is the only right way to represent the mapping between SmartAnthill protocol stack and OSI/ISO".

