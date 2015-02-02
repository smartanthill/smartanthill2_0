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


SmartAnthill: An open IoT system
================================

**SmartAnthill** is an open IoT system which allows easy control over multiple
microcontroller-powered devices, creating a home- or office-wide heterogeneous
network out of these devices.

SmartAnthill system can be pretty much anything: from a system to control
railway network model to an office-wide heating control and security system.
As an open system, SmartAnthill can integrate together a wide range of devices
beginning from embedded development boards and ending with off-the-shelf
sensors and actuators. They can be connected via very different communication
means - from wired (currently Serial, with CAN bus and Ethernet planned soon)
to wireless (currently IEEE 802.15, with low-cost RF, ZigBee and WiFi planned
soon).

All SmartAnthill devices within a system are controlled from the one place
(such as PC or credit-card sized computer Raspberry Pi, BeagleBoard or
CubieBoard), with an optional access via Internet.

Contents
--------

.. toctree::
    :maxdepth: 2

    design-documents/index
    design-documents/reference-implementation/index
