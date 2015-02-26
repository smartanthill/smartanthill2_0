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

.. _sagdp:

SmartAnthill Guaranteed Delivery Protocol (SAGDP)
=================================================

:Version:   v0.1a

*NB: this document relies on certain terms and concepts introduced in*
:ref:`saoverarch` *and*
:ref:`saprotostack` *documents, please make sure to read them before proceeding.*

SAGDP (SmartAnthill Guaranteed Delivery Protocol) aims to provide reliable message delivery for SmartAnthill environments; as described in
:ref:`saoverarch` document, SmartAnthill environments tend to be extremely limited, and tend to require special attention to energy-saving features. In addition, special considerations (such as ability to turn off receiver temporarily) need to be considered.

SAGDP belongs to Layer 2 of OSI/ISO network model, see
:ref:`saprotostack` document for details.

.. contents::

1. Main notions and definitions
-------------------------------

1.1. **Packet**. A unit of data exchange with other levels/protocols. For the sake of clarity two types of packets are distinguished:

     1.1.1. **HLP packet**: a packet that is sent to or received from a high level protocol;

     1.1.2. **UP packet**:  a packet that is sent to or received from an underlying protocol. HLP packet data is a payload data of UP protocol as it will be discussed in more details below.

     1.1.3. **Packet ID (PID)**: each packet has an associated unique (for communication between two given devices) packet ID. It is a responsibility of an underlying protocol to generate an ID,  to report the generated ID to SAGDP, andto send a packet together with the generated ID to a communication peer.

     1.1.4. **Preceding packet ID (PPID)**: a PID of a preceding packet in the chain, if preceding packet exists.
	 
1.2. **Chain**. An ordered set of packets. Each packet in a chain is of one of mutually exclusive types: "first", "intermediate", and "terminating", wherein "first" is the first packet in the chain, "terminating" is the last packet in the chain, and "intermediate" is neither "first" nor "terminating".

1.3. **Master and Slave**. For certain reasons that are discussed below parties participating in data exchange and using this protocol are considered as non-equivalent to each other, and details of protocols at each side are somehow different. To distinguish sides, where applicable, we will use terms Master and Slave. Usually Master is a device generating and sending some commands, and Slave is a device receiving commands and returning results.

1.4. **Error Message**. A packet that represents an error report. This packet can be sent by a Slave in context of any or no chain, if the Slave has encountered an error that prevents it from further packet processing. To be distinguished from other packets, a packet containing Error Message must be marked as both "first" and "terminating" since it has no definite context and does not assume any response.

1.5. **UP packet structure**: UP packet structure looks as follows:
	 
**\| First Byte \| PPID \| HLP packet \|**

where

  * **First Byte** is a 1 byte field that is treated as follows (starting from LSB):

     * **bit 0**: "is-first" flag; set to 1 if a packet is marked as "first", and to 0 otherwise;
     * **bit 1**: "is-terminating" flag; set to 1 if a packet is marked as "terminating", and to 0 otherwise;
     * **bit 2**: "requested-resend" flag; set to 1 if a packet is being re-sent as a result of a repeated receiving of a packet being responded;
     * **Remaining 5 bits**: reserved; must be set to 0.

  * **PPID**: 6-byte field with PPID
  
  * **HLP packet**: variable size field; data that is defined by a higher level protocol.



2. Scenarios
------------

2.1. Normal processing of a packet chain.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two devices, A and B, participate in packet exchange. Each packet sent, except a packet with status "terminating", assumes that there is a packet to be received from the opposite side of communication.

If all packets sent are actually delivered to the other side of communication (that is, no packet is lost on the way), a  "ping-pong" packet exchange happens starting from a packet marked as "first" and ending with a packet marked "terminating". To have guaranteed delivery, if no response to non-"terminating" packet is received, the packet is resent.

In more detail, a device A sends a non-"terminating" packet P to the device B and starts waiting for a packet P' to receive from B. If no packet is received within certain time interval, A resends the packet P to B in hope the packet P will successfully go through. Two main cases are, in general, possible, if A receives no packet from B in turn: (1) packet P is lost, and (2) packet P has been delivered successfully, but packet P' is lost.

In case (1), resending packet P can lead (after one or more repetitions) to reception of P at B. In the same time, while P is not received at B, similar to what A does, B resends its last packet (a predecessor of P in chain). In case (2) B replies by a packet P' to packet P (and does the same to each additional packet P' received (for instance, because of case (1)).

Thus, after sending a packet P, A can get either a reply to P, or a predecessor of P in chain. Details of processing of both options are considered in more details while discussing protocol states and events.

2.1.1. Special case: planned turning-off the receiver.
''''''''''''''''''''''''''''''''''''''''''''''''''''''

In some cases it may be desirable to turn off the receiver of one of devices, for instance, for power saving. Since with a receiver turned off a device could not be able to receive packets (including reply to the last packet sent to the other side of communication), chains must be organized in a way that the last received packet at the side that plans to turn off the receiver, would be "terminating" (that is such that does not assume sending a packet in turn).

2.2. Motivating differences in protocol for Master and Slave side.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Scenario: Two sides, Master and Slave, start their chains at the same time (that is, they send packets that are "first" ones in their respective chains). This could lead to having two chains at the same time, which is an unusual situation for SAGDP and should be handled separately.

Solution. The protocol is asymmetric for participating parties, that is, incoming packets are processed differently for Master and for Slave side. Particularly, if on the Slave side a "first" packet in a chain is received, current processing on the Slave side (if any) is terminated, and processing of a new chain starts. In turn, on the Master side, if a packet that is not in a chain currently processed by Master, is received, it is ignored. In particular, if a packet with status "first" in the chain is received from the Slave as in the discussed scenario, it will be ignored, and the "first" packet of the Master chain will eventually be resent (by timeout). Upon reception on the Slave side, this packet will cause start of the Master chain processing.

2.3. Inconsistency in order of incoming packets within the chain.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Scenario: a packet that is not "first" in a chain received, and the ID of a packet to which it is intended to be a reply does not coincide with the ID of the last sent message. Problem: obvious inconsistency in data exchange. While this shouldn't happen if both parties adhere to the protocol, in real life it is possible due to events such as reboots, power losses, malfunctions etc.

Solution. On the Slave side this causes a device reset (since no reasonable processing can be continued). On the master side such a packet is ignored [+++do we report it to an upper level?]

2.4. Motivating "requested-resend" flag.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO: is 'requested-resend' the same as 'Resent-Packet' below?

Scenario: Side A has sent an "intermediate" packet in a chain to side B, but B has not received it; both sides are waiting for a packet: side A waits for a reply to the packet sent, and size B waits for a reply to a previous packet in the chain. Both sides can re-send respective packets by timeout. A problem could appear, if both sides would send packets by timeout in the same time as this will cause duplicated sending of all remaining packets in the chain.

(Virtual) **Example 1**:

...

S1. A <- B: packet #3

S2. A -> B: packet #4 (reply to #3; lost)

S3. A waits for reply to #4; B waits for reply to #3

S4. A -> B: packet #4 (re-send by timeout); A <- B: packet #3 (re-send by timeout)

S5. A -> B: packet #4 (as reply to packet #3 received at S4.)

S6. A <- B: packet #5 (as reply to packet #4 received at S4.)

S6. A <- B: packet #5 (as reply to packet #4 received at S5.)

...

To avoid such duplication a "requested-resend" flag is set for each packet that is a reply to a packet that is received not a first time. Then the Example 1 is transformed to

(Actual) **Example 2**:

...

S1. A <- B: packet #3

S2. A -> B: packet #4 (reply to #3; lost)

S3. A waits for reply to #4; B waits for reply to #3

S4. A -> B: packet #4 (re-send by timeout); A <- B: packet #3 (re-send by timeout)

S5. A -> B: packet #4 (as reply to packet #3 received at S4. with flag "requested-resend" set)

S6. A <- B: packet #5 (as reply to packet #4 received at S4.)

S6. B does nothing with respect to packet #4 received at S5 as flag "requested-resend" was found

...

Thus a potential for duplicated packet sending is eliminated.



3. States
---------

SAGDP has four states.

3.1. "not initialized"
^^^^^^^^^^^^^^^^^^^^^^
SAGDP appears in this state at system start, and can appear at any time, if detected inconsistencies in packet sequencing are such that the context of processing is lost and all existing data, if any, becomes invalid. The only event that can be processed in this state is "initializing", which results in transition to "idle" state.

This state has no associated data.

3.2. "idle"
^^^^^^^^^^^
If no chain is being processed, the protocol appears in state "idle" and waits for a packet that is marked as a "first" in chain from either a higher level protocol (when the device itself initiates communication) or from an underlying protocol (that is, ultimately, from a device that is a partner for communication). The first case results in transition to "wait-remote" state since after packet sending to the other device a response is being expected and waited. In the second case it is a communication partner device that initiated communication, and implementing device is to respond, so transition happens toward "wait-local" state.

Idle state has no associated data.

3.3. "wait-PID"
^^^^^^^^^^^^^^^^^
When a packet is sent to the communication partner device, a PID is expected to be received in turn. This state can be considered as formal (addressing rather interface problem of getting PID that was just sent) and is logically preceeding to "wait-remote" state. For correct processing transition to this state must happen aerlier that a reply-packet is received from a communication peer, but practically this is not a problem since such transition happens based on processing within the same device while receiving a reply-packet normally assumes communication between different devices. 

"Wait-local" has no associated data.

3.4. "wait-remote"
^^^^^^^^^^^^^^^^^^
When a packet is sent to the communication partner device, a reply packet is expected, and the protocol is in "wait-remote" state. With respect to chain ordering two types of packets can arrive: a reply to the packet sent (which means, in particular, that the last sent packet has been received by a communication partner device), and a previously received packet (which means that the last sent packet has not been delivered successfully). In the first case the payload of the received packet is forwarded to the higher level protocol for processing, and SAGDP transits to "wait-local" state waiting for the reply from the higher level. In the second case a last sent packet is resent, and the protocol remains in the same "wait-remote" state.

Another event that can happen in this state is a timer event. If nothing is received from a communication partner device within certain time period from the last packet has been sent, a last sent packet should be resent. Timer event happens after expiration of that time period. The protocol remains in the same "wait-remote" state after timer event.

"Wait-remote" has the following associated data:

- last sent packet (LSP);
- last sent packet ID (LSPID);
- length of time interval between re-send attempts (RSP).

LSP is used for packet resending, and RSP is used to set timer. LSPID is used to check whether an incoming packet is a reply to the last sent packet by comparison of LSPID with PPID of the received packet.

3.5. "wait-local"
^^^^^^^^^^^^^^^^^
When payload data of a new packet received from the underlying protocol (and thus, ultimately, from a communication partner device) is forwarded to the higher level protocol, SAGDP starts waiting for a reply from a higher level, and stays in "wait-local" state. In this state the only legitimate event is receiving a packet from a higher level that is not marked as a "first" in chain.

"Wait-local" has the following associated data:

- last received packet unique identifier (LRPID),

which is to be added to the header of a packet that is to be forwarded to underlying protocol as an indication to which packet in chain the current packet serves as a reply.

4. Events
---------

All events may be separated into three groups: (1) getting a packet from an underlying protocol (UP packet), (2) getting a packet  from a higher level protocol (HLP packet), and (3) timer event.

Here is a full list of events.

4.1. Receiving an UP packet with flag "New-Packet"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A packet that has not been received ever before arrives. Unless an error in chaining happened, it is either the first in a new chain, or a reply of a communication partner to the last sent packet. This event is initiated by an underlying protocol. In general, a payload of this packet is to be extracted and passed to a higher level protocol.

4.2. Receiving an UP packet with flag "Resent-Packet"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A packet that is identical to last received packet arrives. Regularly it can happen, if a communication partner has not received the last sent packet. This event is initiated by an underlying protocol. In general, a last sent

4.3. Receiving an HLP packet that is "first", or is "intermediate", or is "terminating"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO: pls check that the intended meaning didn't change

A packet from an higher level protocol has been received with a respective status in chain. This packet is to be pre-processed and passed to an underlying protocol to be ultimately sent to a communication partner device.

4.4. Receiving PID
^^^^^^^^^^^^^^^^^^
See comments to "wait-PID" state.

4.5. Timer
^^^^^^^^^^
In the context of SAGDP timer event is used for packet resending, if a response has not been received within certain time.


5. Event processing
-------------------

In short, to process events from the first group (receiving an UP packet) the protocol should be in either "idle" or "wait-remote" state. To process events from the second group (receiving an HLP packet) the protocol should be in either "idle" or "wait-local" state. To process timer events the protocol should be in "wait-remote" state. Detailed description is placed below.


5.1. Processing events in idle state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In idle state SAGDP is ready to accept a packet marked as "first" from either underlying or higher level protocol.

5.1.1. Receiving an UP packet with flag "New-Packet"
''''''''''''''''''''''''''''''''''''''''''''''''''''

Processing of this event is different at Mater's and Slave's side in a part when the packet is not a subsequent packet within a current chain.

**At Master's side**, processing depends on the status of the packet in chain.
  * Error Message: payload of the packet is reported to a higher level protocol with its status, and SAGDP changes its state to idle.
  * "First": packet PID is saved as a current value of LRPID, payload of the packet is reported to a higher level protocol with its status, and SAGDP changes its state to wait-local.
  * "Intermediate": unexpected, ignored [+++check]
  * "Terminating": unexpected, ignored [+++check]

**At Slave side**,
  * "First": packet PID is saved as a current value of LRPID, payload of the packet is reported to a higher level protocol with its status, and SAGDP changes its state to wait-local.
  * Error Message, "Intermediate", "Terminating": unexpected; system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.

5.1.2. Receiving an HLP packet that is "first"
''''''''''''''''''''''''''''''''''''''''''''''

An UP packet is formed wherein HLP packet becomes a payload data, and a header contains flags regarding the position of the packet in chain ("is-first" flag is set, "is-last" is not set) and the packet PPID that is equal to LRPID. The UP packet is saved as LSP. Timer is set to RSP. The UP packet is sent to the underlying protocol. SAGDP changes its state to "wait-PID".

5.1.3. Receiving an UP packet with flag "Resent-Packet"; or an HLP packet that is "intermediate"; or an HLP packet that is "terminating"
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

TODO: pls check that the intended meaning didn't change

If any of these events happen in idle state, consistency of data processing is broken. If implemented on Master, an error must e reported to the higher level protocol, and SAGDP transits to "idle" state. If implemented on Slave, system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.

5.1.4. Timer; or Receiving PID
''''''''''''''''''''''''''''''

Ignored in this state.


5.2. Processing events in wait-local state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In wait-local state SAGDP waits from a higher level protocol for a packet that is not a "first" in the chain.

5.2.1. Receiving an HLP packet that is "intermediate"
'''''''''''''''''''''''''''''''''''''''''''''''''''''

An UP packet is formed wherein HLP packet becomes a payload data, and a header contains flags regarding the position of the packet in chain ("is-first" flag is not set, "is-last" is not set) and the packet PPID that is equal to LSPID. The UP packet is saved as LSP. Timer is set to RSP. The UP packet is sent to the underlying protocol. SAGDP changes its state to "wait-PID".

5.2.2. Receiving an HLP packet that is "terminating"
''''''''''''''''''''''''''''''''''''''''''''''''''''

An UP packet is formed wherein HLP packet becomes a payload data, and a header contains flags regarding the position of the packet in chain ("is-first" flag is not set, "is-last" is not set) and the packet PPID that is equal to LSPID. The UP packet is sent to the underlying protocol. SAGDP changes its state to "idle".

5.2.3. Receiving a UP packet with flag "Resent-Packet"
''''''''''''''''''''''''''''''''''''''''''''''''''''''

TODO: pls check additionally!!!!

The LSP is sent to the underlying protocol. Timer is set to RSP.

5.2.4. Receiving an HLP packet that is "first"; or an UP packet with flag "New-Packet"; or Receiving PID
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

TODO: pls check that the intended meaning didn't change

If any of these events happen in wait-local state, consistency of data processing is broken. If implemented on Master, an error must e reported to the higher level protocol, and SAGDP transits to "idle" state. If implemented on Slave, system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.

5.2.4. Timer
''''''''''''

Ignored in this state.


5.3. Processing events in wait-PID state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

5.3.1. Receiving PID
''''''''''''''''''''

LSPID is set to the received PID.  SAGDP changes its state to "wait-remote".

5.3.2. Timer
''''''''''''

Ignored in this state.

5.3.3. Receiving an HLP packet that is "first"; or receiving an HLP packet that is "intermediate"; or receiving an HLP packet that is "terminating"
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

If any of these events happen in wait-remote state, consistency of data processing is broken. If implemented on Master, an error must e reported to the higher level protocol, and SAGDP transits to "idle" state. If implemented on Slave, system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.


5.4. Processing events in wait-remote state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

5.4.1. Receiving an UP packet with flag "New-Packet"
''''''''''''''''''''''''''''''''''''''''''''''''''''

Processing of this event is different at Mater's and Slave's side in a part when the packet is not a subsequent packet within a current chain.

**At Master's side**, processing depends on the status of the packet in chain.
  * Error Message: payload of the packet is reported to a higher level protocol with its status, and SAGDP changes its state to idle.
  * "First":  unexpected, ignored [+++check]
  * "Intermediate": chain consistency is verified by comparison of PPID of the packet with LSPID.
     * PPID is equal to LSPID (received packet is a response to the last sent packet): packet PID is saved as a current value of LRPID, payload of the packet is reported to a higher level protocol with its status in chain, and SAGDP changes its state to wait-local.
     * PPID is not equal to LSPID (chain is broken): the packet is ignored.
  * "Terminating": chain consistency is verified by comparison of PPID of the packet with LSPID.
     * PPID is equal to LSPID (received packet is a response to the last sent packet): payload of the packet is reported to a higher level protocol with its status in chain, and SAGDP changes its state to idle.
     * PPID is not equal to LSPID (chain is broken): the packet is ignored  [+++check]

**At Slave side**,
  * Error Message, "First": unexpected; system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.
  * "Intermediate": chain consistency is verified by comparison of PPID of the packet with LSPID.
     * PPID is equal to LSPID (received packet is a response to the last sent packet): packet PID is saved as a current value of LRPID, payload of the packet is reported to a higher level protocol with its status in chain, and SAGDP changes its state to wait-local.
     * PPID is not equal to LSPID (chain is broken): system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.
  * "Terminating": chain consistency is verified by comparison of PPID of the packet with LSPID.
     * PPID is equal to LSPID (received packet is a response to the last sent packet): payload of the packet is reported to a higher level protocol with its status in chain, and SAGDP changes its state to idle.
     * PPID is not equal to LSPID (chain is broken): system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.

5.4.2. Receiving an UP packet with flag "Resent-Packet"
'''''''''''''''''''''''''''''''''''''''''''''''''''''''

TODO: pls check additionally (old version: "The LSP is sent to the underlying protocol. Timer is set to RSP."; reason to change: in wait-remote SAGDP can only get a new packet as then it will transit to wait-local state)!!!!

TODO: when checked, merge with 5.4.4.

If this events happens in wait-remote state, consistency of data processing is broken. If implemented on Master, an error must be reported to the higher level protocol, and SAGDP transits to "idle" state. If implemented on Slave, system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.


5.4.3. Timer
''''''''''''

The LSP is sent to the underlying protocol. Timer is set to RSP.

5.4.4. Receiving an HLP packet that is "first"; or receiving an HLP packet that is "intermediate"; or receiving an HLP packet that is "terminating"
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

If any of these events happen in wait-remote state, consistency of data processing is broken. If implemented on Master, an error must be reported to the higher level protocol, and SAGDP transits to "idle" state. If implemented on Slave, system must send a packet with Error Message to its communication partner and then to transit to "not initialized" state thus invalidating all current data.

5.4.5. Receiving PID
''''''''''''''''''''

If this event happens in the wait-remote state, it means that SAGDP ir de-sychronized with an underlying protocol, and the system must be reset [TODO: is there any reasonable case for that?]





[+++ processing around "requested-resend" flag]



... [work in progress]
