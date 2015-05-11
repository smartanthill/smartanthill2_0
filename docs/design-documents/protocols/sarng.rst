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

.. _sarng:

SmartAnthill SmartAnthill Random Number Generation and Key Generation
=====================================================================

:Version:   v0.1.4

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

Random Number Generation is vital for ensuring security. This document describes requirements for Random Number Generation for SmartAnthill Devices.

.. contents::

Poor-Man's PRNG
---------------

Each device with Poor-Man's PRNG, has it's own AES-128 secret key (this key MUST NOT be stored outside of the device), and additionally keeps a counter. This counter MUST be kept in a way which guarantees that the same value of the counter is never reused; this includes both having counter of sufficient size, and proper commits to persistent storage to avoid re-use of the counter in case of accidental device reboot. As for commits to persistent storage - two such implementations are discussed in :ref:`sasp` document, in 'Implementation Details' section, with respect to storing nonces.

Then, Poor-Man's PRNG simply encrypts current value of the counter with AES-128, increments counter (see note above about guarantees of no-reuse), and returns encrypted value of the counter as next 16 bytes of the random output.

Devices with pre-initialized Poor-Man's PRNG
--------------------------------------------

Resource-constrained SmartAnthill Devices which don't have their own crypto-safe RNG, MUST use Poor-Man's PRNG. On such Devices, Poor-Man's PRNG MUST be pre-initialized with a random key and random initial counter, generated outside of Device. Both key and counter MUST be crypto-safe random numbers.

Such Devices are known as Unique Devices.

non-Secure Devices without pre-initialized Poor-Man's PRNG
----------------------------------------------------------

For non-Secure Devices without pre-initialized Poor-Man's PRNG, the following approach ("hardware-assisted Fortuna PRNG") is allowed.

* Device MUST implement Fortuna PRNG, with multiple entropy sources to feed to Fortuna as described below

  + when "feeding entropy to Fortuna", exact bit representation doesn't matter, as long as all the data bits are fed to TBD function of Fortuna
  + TODO: potential simplifications (while staying strict Fortuna under restricted circumstances)
  + TODO: persistent Fortuna state

* Device MUST have at least one MCU ADC channel which is either connected to an entropy source (such as Zener diode, details TBD), or just being not connected at all. This ADC is named "noise ADC"

  + it is acceptable to disconnect ADC channel only temporarily (for example, using an analogue switch); in this case, ADC channel MUST be disconnected for the whole duration of RNG initialization (i.e. it is not acceptable to disconnect it only for one measurement and to connect it back right afterwards).

* During each "pairing" (IMPORTANT: it applies to any "pairing", not just first "pairing"), the following procedure of RNG additional seeding MUST be performed:

  + When pairing procedure starts, Device MUST initialize two internal variables (Network-Time-VonNeumann-Count and ADC-VonNeumann-Count) as zeros
  + Device MUST implement "Entropy Gathering" procedure as defined in :ref:`sapairing` document

  + On receiving each packet with entropy, Device MUST:

    - feed received ENTROPY to the Fortuna
    - feed entropy which is based on pseudo-time since the request has been sent, with at least 1mks precision; for this purpose, exact time isn't important, what is important is that two different times with 1mks difference produce two different results

      * in particular, time MAY be pseudo-measured using "tight loops" (increment-pseudo-time-check-packet-arrival-repeat-until-packet-arrives), provided that 1mks requirement is satisfied
      * if pseudo-measured time is different from last pseudo-measured time, increment Network-Time-VonNeumann-Count. NB: even if Network-Time-VonNeumann-Count is not incremented, the data SHOULD be fed to Fortuna PRNG

  + in addition, if bare-metal implementation is used, and packet arrives via an interrupt, Device SHOULD add "program-counter-before-interrupt has been called" (which is usually readily available as `[SP-some_constant]`) to Fortuna
  + in addition, all interrupts SHOULD be handled in similar manner
  + Device MUST continue "Entropy Gathering" procedure at least until Network-Time-VonNeumann-Count reaches 255.
  + in addition, Device MUST perform measurements of "noise ADC" and feed the results to the Fortuna PRNG

    - on every such measurement, if measurement result is neither maximun nor minimum possible value for the ADC in question, *and* measurement result doesn't match previous measurement from "noise ADC", ADC-VonNeumann-Count is incremented. NB: even if ADC-VonNeumann-Counter is not incremented, entropy still SHOULD be fed to Fortuna PRNG
    - these measurements MAY be performed in parallel with "Entropy Gathering" network exchange, or separately

  + in addition, Device SHOULD perform measurements of all the other ADCs in the system (e.g. one measurement for each other ADC for one measurement of "noise ADC") and feed the results to Fortuna PRNG
  + Device MUST continue measurements of "noise ADC" at least until ADC-VonNeumann-Count reaches 255.
  + after both ADC-VonNeumann-Count and Network-Time-VonNeumann-Count reach 255, Device MAY decide to complete RNG additional seeding
  + to complete RNG additional seeding, Device MUST skip at least TODO bits of Fortuna output

* Until RNG additional seeding is completed, RNG output MUST NOT be used in any manner
* after RNG additional seeding is completed, Devices still SHOULD feed all the available entropy (as described above) to the Fortuna PRNG

Fortuna State and re-pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When Device is to be re-paired (i.e. Device pairing state is changed to PRE-PAIRING, see :ref:`sapairing` document for details), Fortuna PRNG stage (both persistent and in-memory) MUST NOT be affected. The only process which MAY rewrite Fortuna persistent state while ignoring the existing Fortuna state, is Device re-programming (but **not** OtA re-programming).

Secure Devices without pre-initialized Poor-Man's RNG
-----------------------------------------------------

While it is NOT RECOMMENDED, Secure SmartAnthill Devices MAY be implemented without pre-initialized Poor-Man's RNG. In this case:

* Device MUST have a hardware entropy source, which provides a hardware-generated bit stream
* Device MUST implement on-line testing of hardware-generated bit stream (monobit test, poker test, runs test, and long runs test, as they were specified in FIPS140-2 after Change Notice 1 and before Change Notice 2; testing should be performed on each 20000-bit block before this block is fed to Fortuna). TODO: adaptation to streaming?
* on-line testing MUST be performed on a bit stream before any cryptographic primitives are applied (but SHOULD be performed after von Neumann bias removal)
* Device MUST implement Fortuna PRNG (as specified above). 

  + this includes implementing Fortuna persistent state as described above

* on the first launch, at least 1 hardware-generated bit stream block, with on-line test above being successful, MUST be fed to a Fortuna PRNG during Fortuna initialization (after 20000-bit blocks pass on-line testing).

  + until such an initialization is completed, Device MUST NOT be operational
  + bit stream blocks with test failed, still SHOULD be fed to Fortuna PRNG
  + RNG MUST skip at least first TODO bits of the Fortuna output bit stream (before starting to output Fortuna output as RNG output)

* Device MUST continue feeding output from hardware entropy source to Fortuna PRNG, without applying the online tests, at a rate at least 1 bit per second (as long as Device is running during at least some portion of the 1 second and not in a hardware sleep mode)
* Device SHOULD feed additional available entropy (timings, ADC etc. as described above) to Fortuna PRNG

Devices with both pre-initialized Poor-Man's RNG and Fortuna PRNG
-----------------------------------------------------------------

Where possible, Devices SHOULD implement both pre-initialized Poor-Man's PRNG and harware-assisted Fortuna PRNG (the latter as described in "non-Secure Devices without pre-initialized Poor-Man's PRNG" section). In such cases, to obtain one byte of output bit stream, RNG MUST take one byte from Fortuna output, and XOR it with one byte of Poor-Man's PRNG output 

Secure Devices
--------------

SmartAnthill Secure Devices SHOULD use both pre-initialized Poor-Man's RNG and hardware-assisted Fortuna PRNG (the latter - as described in "non-Secure Devices without pre-initialized Poor-Man's PRNG" section, or - RECOMMENDED - as described in "Secure Devices without pre-initialized Poor-Man's RNG" section). 

SmartAnthill Client (and Devices with Crypto-Safe RNG)
------------------------------------------------------

Even if the system where the SmartAnthill stack is running, has a supposedly crypto-safe RNG (such as built-in crypto-safe /dev/urandom), SmartAnthill implementations still MUST employ Poor-Man's PRNG (as described above) in addition to system-provided crypto-safe PRNG. In such cases, each byte of SmartAnthill RNG (which is provided to the rest of SmartAnthill) SHOULD be a XOR of 1 byte of system-provided crypto-safe PRNG, and 1 byte of Poor-Man's PRNG. 

*Rationale. This approach allows to reduce the impact of catastrophic failures of the system-provided crypto-safe PRNG (for example, it would mitigate effects of the Debian RNG disaster very significantly).*

To initialize Poor-Man's RNG on Client side, SmartAnthill implementation MUST NOT use the same crypto-safe RNG which output will be used for XOR-ing with Poor-Man's RNG (as specified above); instead, Poor-Man's RNG on Client side MUST be initialized independently; valid examples of such independent initialization include XOR-ing of at least two sources, such as an independent Fortuna PRNG with user input (timing of typing or mouse movements), or online generators such as 'raw bytes' from random.org or from smartanthill.org (TODO); IMPORTANT: all exchanges with online generators MUST be over https, and with server certificate validation.

The same procedure SHOULD also be used for generating random data which is used for SmartAnthill key generation.

Key Generation
--------------

This sections describes rules for generating keys (and other key material, such as DH random numbers).

For Devices which support OtA Pairing (see :ref:`sapairing` document for details), key material needs to be generated. For such Devices the following requirements MUST be met:

* if Device doesn't have Fortuna PRNG:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.
  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - calculate `output=AES(key=KEY4KEYS,data=POORMAN4KEYS.Random16bytes())`

* if Device does have a hardware-assisted Fortuna PRNG but doesn't have pre-initialized keys:

  + Fortuna output (after mandatory RNG additional seeding as described above) is used as a key material

* if Device has both pre-initialized keys and hardware-assisted Fortuna PRNG:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.
  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + Fortuna output (after mandatory RNG additional seeding as described above) is used for key material generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - calculate `output=Fortuna.Random16bytes() XOR AES(key=KEY4KEYS,data=POORMAN4KEYS.Random16bytes())`

* if Device (or Client) has a crypto-safe RNG:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.

    - Initialization of both Poor-Man's PRNGs (as well as initialization of KEY4KEYS and POORMAN4KEYS, see below) MUST be done independently, as specified in "SmartAnthill Client (and Devices with Crypto-Safe RNG)" section above.

  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - calculate `output=CryptoSafeRNG.Random16bytes() XOR AES(key=KEY4KEYS,data=POORMAN4KEYS.Random16bytes())`

Non-Key Random Stream
---------------------

SmartAnthill RNG provides a 'non-key Random Stream' for various purposes such as padding, ENTROPY data for the pairing (sic!), etc. Generation of 128 bits of non-key Random Stream is similar to key generation described above, with the following differences:

* instead of POORMAN4KEYS Poor-Man's PRNG, NONKEYPOORMAN Poor-Man's PRNG is used
* instead of AES(key=KEY4KEYS,data=DATA), DATA is used directly

