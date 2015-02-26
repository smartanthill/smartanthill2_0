SmartAnthill 2.0
================

.. image:: https://travis-ci.org/smartanthill/smartanthill2_0.svg?branch=develop
    :target: https://travis-ci.org/smartanthill/smartanthill2_0
    :alt: Build Status
.. image:: https://coveralls.io/repos/smartanthill/smartanthill2_0/badge.svg
    :target: https://coveralls.io/r/smartanthill/smartanthill2_0
.. image:: https://gemnasium.com/smartanthill/smartanthill1_0.svg
    :target: https://gemnasium.com/smartanthill/smartanthill1_0
    :alt: Dependency Status
.. image:: https://pypip.in/version/smartanthill/badge.png
    :target: https://pypi.python.org/pypi/smartanthill/
    :alt: Latest Version
.. image:: https://pypip.in/license/smartanthill/badge.png?v2
    :target: https://pypi.python.org/pypi/smartanthill/
    :alt:  License

**SmartAnthill** is an open IoT system which allows easy control over multiple
microcontroller-powered devices, creating a home- or office-wide heterogeneous
network out of these devices.

SmartAnthill system can be pretty much anything: from a system to control
railway network model to an office-wide heating control and security system.
As an open system, SmartAnthill can integrate together a wide range of devices
beginning from embedded development boards and ending with off-the-shelf
sensors and actuators. They can be connected via very different communication
means - from wired (currently Serial, with CAN bus and Ethernet planned soon)
to wireless (currently IEEE 802.15.4, with low-cost RF, Bluetooth Smart,
ZigBee and WiFi planned soon).

All SmartAnthill devices within a system are controlled from the one place
(such as PC or credit-card sized computer Raspberry Pi, BeagleBoard or
CubieBoard), with an optional access via Internet.

`Documentation <http://docs.smartanthill.org>`_
------------------------------------------------

* `Specification <http://docs.smartanthill.org/en/latest/design-documents/index.html>`_
    - `Overall Architecture <http://docs.smartanthill.org/en/latest/design-documents/smartanthill-overall-architecture.html>`_
    - `Core Architecture <http://docs.smartanthill.org/en/latest/design-documents/smartanthill-core-architecture.html>`_
    - `Protocols <http://docs.smartanthill.org/en/latest/design-documents/protocols/index.html>`_
    - `Yocto VM <http://docs.smartanthill.org/en/latest/design-documents/protocols/yocto-vm.html>`_
* `Reference Implementation <http://docs.smartanthill.org/en/latest/design-documents/reference-implementation/index.html>`_
    - `MCU Software Architecture <http://docs.smartanthill.org/en/latest/design-documents/reference-implementation/mcu/smartanthill-reference-mcu-software-architecture.html>`_


.. image:: https://raw.githubusercontent.com/smartanthill/smartanthill2_0/develop/docs/_static/diagrams/smartanthill-overall-architecture-diagram.png
    :alt: SmartAnthill Overall Architecture
