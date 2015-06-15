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

:Version:   v0.1.5b

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *and* :ref:`saprotostack` *documents, please make sure to read them before proceeding.*

Random Number Generation is vital for ensuring security. This document describes requirements for Random Number Generation for SmartAnthill Devices.

.. contents::

Poor-Man's PRNG
---------------

Each device with Poor-Man's PRNG, has it's own AES-128 secret key (this key MUST NOT be stored outside of the device), and additionally keeps a counter. This counter MUST be kept in a way which guarantees that the same value of the counter is never reused; this includes both having counter of sufficient size, and proper commits to persistent storage to avoid re-use of the counter in case of accidental device reboot. As for commits to persistent storage - two such implementations are discussed in :ref:`sasp` document, in 'Implementation Details' section, with respect to storing nonces.

Then, Poor-Man's PRNG simply encrypts current value of the counter with AES-128, increments counter (see note above about guarantees of no-reuse), and returns encrypted value of the counter as next 16 bytes of the random output.

Devices with uniquely-pre-initialized Poor-Man's PRNG
-----------------------------------------------------

Resource-constrained SmartAnthill Devices which don't have their own crypto-safe RNG, MUST use Poor-Man's PRNG. On such Devices, Poor-Man's PRNG MUST be pre-populated during Device manufacturing, with a random key and random initial counter, generated outside of Device. Both key and counter MUST be crypto-safe random numbers, and MUST be statistically unique for each Device.

Devices with hardware-assisted Fortuna
--------------------------------------

If Device doesn't have a uniquely-pre-initialized Poor-Man's PRNG, the following approach based on hardware-assisted Fortuna PRNG, MAY be used (ONLY for certain types of Devices, see 'Restrictions for Secure and non-Secure Devices' section below). For such Devices with hardware-assisted Fortuna, the following conditions MUST be met:

* Device MUST implement Fortuna PRNG, with multiple entropy sources to feed to Fortuna as described below

  + details and implementation options are specified below
  + Device MUST comply to seed file requirements as specified below

* Device MUST implement hardware entropy gathering, and RNG additional seeding procedure, as described below


Fortuna Implementation in SmartAnthill
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are two approaches to implement Fortuna in SmartAntill: 'Radical' and 'Conservative'. 'Radical' is not strictly compliant with Fortuna description from [Fortuna], but we feel it should perform significantly better for our special circumstances. 'Conservative' is fully compliant to description in [Fortuna], with really minor tweaks (within the spirit of Fortuna) to reduce resource requirements. Currently, and until it is shown otherwise, both implementations are acceptable for SmartAnthill.

In any case, pool size for SmartAnthill Fortuna implementations is 128*3 bytes; effectively it means that we're making a  guesstimate that each event (encoded as 3 bytes per ADDRANDOMEVENT() description) carries one bit of entropy.

Currently, Fortuna implementation is estimated to require 32 (state of first SHA256 in SHAd256)+32 (state of second SHA256 in SHAd256)+64 (512-bit chunk buffer) = 128 bytes per pool, plus 32 bytes regardless of pools (generator state).

'Radical' SmartAnthill Fortuna
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

'Radical' SmartAnthill Fortuna has the following changes from [Fortuna] description:

* only one pool is used. *Rationale. Under conditions of generic PC-based RNG it may be seen as a major deficiency, but we feel that for SmartAnthill purposes, where the mostly important random generation (the one for pairing purposes) is 'imminent', and long-term recovery is of significantly less interest than making key material really random. Under these circumstances, spreading entropy across multiple pools, where it won't be used for imminent security-critical key generation, is considered a waste.*

'Conservative' SmartAnthill Fortuna
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

'Conservative' SmartAnthill Fortuna has the following changes from [Fortuna] description:

* for non-Secure SmartAnthill Devices, number of pools MAY be reduced to 16 (from 32 in original Fortuna); for Secure SmartAnthill Devices number of pools MUST be at least 24
* minimum time between reseeds MUST be increased to 1 minute (from 100ms in original Fortuna). *Rationale: given our limited entropy sources and rare events, we're not likely to get 128 bits of entropy more frequently anyway*

These changes bring time-needed-for-attacker-to-exhaust-pools from 13 years as in original Fortuna, down to 1.5 months; we feel that this number is prudent enough for non-Secure devices. For Secure Devices 24 pools with 1 minute minimum reseeds, provide 31 years. 

Fortuna Seed File
^^^^^^^^^^^^^^^^^

[Fortuna] specifies a 64-byte 'seed file' to keep Fortuna state between reboots. SmartAnthill Fortuna implementations MUST implement a 'seed file' (normally in EEPROM), with all atomicity requriements specified in [Fortuna]. If 'seed file' cannot be read on Device start, then Device MUST perform the following (depending on Device 'pairing state' as described in :ref:`sapairing` document):

* if Device is in PRE-PAIRING state, necessary entropy will be gathered during normal "pairing" procedure, so Fortuna may start without seed file.
* if Device is in PAIRING-MITM-CHECK state, Device MUST switch to PRE-PAIRING state and require "pairing" to be repeated (TODO: analyze Client-side errors and user messages)
* if Device is in PAIRING-COMPLETED state, Device MUST perform "entropy gathering" SACCP procedure (not to be confused with 'Entropy Gathering' during "pairing"; TODO!: SACCP packets for this purpose). 

Fortuna 'seed file' MUST be written before any MCUSLEEP operation (TODO: what if MCUSLEEP is memory-preserving?), and MUST be written at least every 10 minutes of Device operation.

Fortuna uniquely-pre-initialized seed file
''''''''''''''''''''''''''''''''''''''''''

To improve security, Devices MAY pre-populate Device with Fortuna seed file during manufacturing; if implemented, this seed file MUST be a file consisting of 64 random crypto-safe bytes. Presence of uniquely-pre-initialized "seed file" does NOT ease any of the other requirements to Fortuna and/or random number generation.

Device Operation for Devices with hardware-assisted Fortuna
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

NB: when "feeding entropy to Fortuna", exact bit representation doesn't matter, as long as all the data bits are fed to ADDRANDOMEVENT() Fortuna function

* Device MUST have at least one MCU ADC channel which is either connected to an entropy source (such as Zener diode, details TBD), or just being not connected at all. This ADC is named "noise ADC"

  + it is acceptable to disconnect ADC channel only temporarily (for example, using an analogue switch); in this case, ADC channel MUST be disconnected for the whole duration of RNG additional seeding (i.e. it is not acceptable to disconnect it only for one measurement and to connect it back right afterwards).

* During each "pairing" (IMPORTANT: it applies to any "pairing", not just first "pairing"), the following procedure of RNG additional seeding MUST be performed:

  + When pairing procedure starts, Device MUST initialize two internal variables (Network-Time-Change-Count and ADC-Change-Count) as zeros
  + Device MUST implement "Entropy Gathering" procedure as defined in :ref:`sapairing` document

  + On receiving each packet with entropy, Device MUST:

    - feed received ENTROPY to the Fortuna (NB: this ENTROPY is not really required, but it costs pretty much nothing to add it, and in case if attacker missed at least a part of the exchange, it certainly improves security, even if all the hardware entropy data turns out to be 100% deterministic, which shouldn't really happen, but...)
    - feed entropy which is based on pseudo-measured time since the request has been sent, with at least 1mks precision; for the purposes of pseudo-measurement of time, exact time isn't important, what is important is that two different times with 1mks difference, produce two different results with a probability at least 50%.

      * in particular, time MAY be pseudo-measured using "tight loops" (increment-pseudo-time-check-packet-arrival-repeat-until-packet-arrives), provided that 1mks requirement is satisfied (i.e. that "tight loop" time is less than 1mks, i.e. `MCU-frequency * tight-loop-clock-count < 1mks`). Device MAY perform some non-time-measured operations (for example, some measurements and/or calculations) after sending a packet and before going into time-pseudo-measuring "tight loop", as long as `maximum-possible-time-before-tight-loop < minimum-possible-packet-round-trip-time`.
      * if pseudo-measured time is different from last pseudo-measured time, increment Network-Time-Change-Count. NB: even if Network-Time-Change-Count is not incremented, time data SHOULD still be fed to Fortuna PRNG
      * additionally, if another independent timer (such as WDT on AVR) is available, it SHOULD be read on packet arrival, and the data from the timer SHOULD be fed to Fortuna PRNG

  + in addition, if bare-metal implementation is used, whenever an interrupt happens (this includes interrupt on receiving packets, and/or any other interrupts), Device SHOULD feed "program-counter-before-interrupt has been called" (which is usually readily available as `[SP-some_constant]`, and usually has 1 or more bits of entropy if the MCU is actively running at the moment) to Fortuna PRNG.

    - regardless of handling interrupts in such a manner, Device still MUST pseudo-measure time in a tight loop as described above
    - in addition, if another independent timer (such as WDT on AVR) is available, it SHOULD be read on all the interrupts, and the data from the timer SHOULD be fed to Fortuna PRNG. If independent timer is read-and-fed-to-Fortuna on interrupt, and all packet arrivals are handled via interrupts, then independent timer SHOULD NOT be read-and-fed-to-Fortuna outside of interrupt (tight-loop pseudo-measure of time outside of interrupt is still necessary)
    - to pass entropy from interrupt handler to Fortuna, entropy MAY be combined within different calls to interrupt handlers; in particular, the entropy MAY be accumulated via XOR-ing (with or without rotations, or using some other mixing function which doesn't affect bit balance; good mixing functions examples include addition/substraction modulo 2^n, XOR, rotations, CRC functions, and crypto hash functions; bad examples include AND,OR, and shifts without rotations which may lose informaiton from some bits completely) incoming entropy in a fixed-size buffer until it is atomically-read-and-removed-from-fixed-size-buffer (TODO: is atomicity strictly required here?) outside of the interrupt handler and is fed to Fortuna PRNG. Regardless of mixing function, implementations MUST provide DEBUG compile-time flag which will ensure that each entropy component is passed separately without any mixing, and is never overwritten until it is read-and-removed; this is necessary to validate implementation to return what is expected (PC and/or timer) and to evaluate amount of entropy they produce.

  + Device MUST continue "Entropy Gathering" procedure at least until Network-Time-Change-Count reaches `250 * number-of-Fortuna-pools`.
  + in addition, Device MUST perform measurements of "noise ADC" and feed the results to the Fortuna PRNG

    - on every such measurement, if measurement result is neither maximum nor minimum possible value for the ADC in question (usually, but not necessarily, minimum is all-zeros, and maximum is all-ones), *and* measurement result doesn't match previous measurement from "noise ADC", ADC-Change-Count is incremented. NB: even if ADC-Change-Count is not incremented, entropy still SHOULD be fed to Fortuna PRNG. NB2: "neither maximum nor minimum" requirement effectively rules out using 1-bit ADCs as "noise ADCs". 
    - these measurements MUST be performed in parallel with "Entropy Gathering" network exchange; at least one ADC measurement per "Entropy Gathering" packet MUST be performed; more than one is fine.

  + in addition, Device SHOULD perform measurements of all the other ADCs in the system (e.g. one measurement for each other ADC for one measurement of "noise ADC") and feed the results to Fortuna PRNG
  + Device MUST continue measurements of "noise ADC" at least until ADC-Change-Count reaches `250 * number-of-Fortuna-pools`.

  + if hardware RNG (for example, accessible via a special MCU instruction) is available, Device SHOULD feed it's output to Fortuna

  + after both ADC-Change-Count and Network-Time-Change-Count reach 250, Device MAY decide to complete RNG additional seeding
  + to complete RNG additional seeding, Device MUST explicitly call Fortuna's RESEED() (see [Fortuna] for details), and then MUST skip at least TODO bits of Fortuna output

* Until RNG additional seeding is completed, RNG output MUST NOT be used in any manner
* after RNG additional seeding is completed, Devices still SHOULD feed all the available entropy (as described above) to the Fortuna PRNG

Fortuna State and re-pairing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When Device is to be re-paired (i.e. Device pairing state is changed to PRE-PAIRING, see :ref:`sapairing` document for details), Fortuna PRNG state (both seed file and in-memory state) MUST NOT be affected. The only process which MAY rewrite Fortuna persistent state while ignoring the existing Fortuna state, is Device re-programming (but **not** OtA re-programming).

Devices with hardware RNG
-------------------------

To qualify as a 'Device with hardware RNG', Device MUST comply with all the following requirements:

* Device MUST have a hardware entropy source, which provides a hardware-generated bit stream
* Device MUST implement on-line testing of hardware-generated bit stream (monobit test, poker test, runs test, and long runs test, as they were specified in FIPS140-2 after Change Notice 1 and before Change Notice 2; testing should be performed on each 20000-bit block before this block is fed to Fortuna). TODO: adaptation to streaming?
* on-line testing MUST be performed on a bit stream before any cryptographic primitives are applied (but SHOULD be performed after von Neumann bias removal)
* Device MUST implement Fortuna PRNG (as specified above). 

  + this includes implementing Fortuna seed file as described above

* on the first launch of the Device (i.e. if Fortuna seed file is not present, and Device is in PRE-PAIRING state), at least 3 of hardware-generated bit stream blocks, with on-line test above being successful, MUST be fed to a Fortuna PRNG during Fortuna initialization:

  + until such an initialization is completed, Device MUST NOT be operational
  + bit stream blocks with online test failed, still SHOULD be fed to Fortuna PRNG
  + RNG MUST skip at least first TODO bits of the Fortuna output bit stream (before starting to output Fortuna output as RNG output)
  
* Device MUST continue feeding output from hardware entropy source to Fortuna PRNG, without applying the online tests, at a rate at least 1 bit per second (as long as Device is running during at least some portion of the 1 second and not in a hardware sleep mode)
* Device SHOULD feed additional available entropy (timings, ADC etc. as described above) to Fortuna PRNG

Restrictions for Secure and non-Secure Devices
----------------------------------------------

non-Secure SmartAnthill Devices MAY use one of the following RNGs (as long as all requirements for respective RNG, as specified above, are complied with):

* uniquely-pre-initialized Poor-Man's PRNGs
* hardware-assisted Fortuna
* hardware-assisted Fortuna with uniquely-pre-initialized seed file
* hardware RNG
* hardware RNG with Fortuna having uniquely-pre-initialized seed file

Secure SmartAnthill Devices MAY use one of the following RNGs (as long as all requirements for respective RNG, as specified above, are complied with):

* uniquely-pre-initialized Poor-Man's PRNGs
* hardware-assisted Fortuna
* hardware-assisted Fortuna with uniquely-pre-initialized seed file
* hardware RNG
* hardware RNG with Fortuna having uniquely-pre-initialized seed file (RECOMMENDED)

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

* if Device doesn't have a hardware-assisted Fortuna PRNG:

  + Device MUST implement at least two uniquely-pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.
  + in addition, Device MUST have an additional uniquely-pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key material, the following procedure applies:

    - calculate `output=AES(key=KEY4KEYS,data=POORMAN4KEYS.Random16bytes())`

* if Device does have a hardware-assisted Fortuna PRNG:

  + Fortuna output (after mandatory RNG additional seeding as described above) is used as a key material

* if Device (or Client) has a crypto-safe RNG:

  + Device MUST implement at least two uniquely-pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.

    - Initialization of both Poor-Man's PRNGs (as well as initialization of KEY4KEYS and POORMAN4KEYS, see below) MUST be done independently, as specified in "SmartAnthill Client (and Devices with Crypto-Safe RNG)" section above.

  + in addition, Device MUST have an additional uniquely-pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - calculate `output=CryptoSafeRNG.Random16bytes() XOR AES(key=KEY4KEYS,data=POORMAN4KEYS.Random16bytes())`

Non-Key Random Stream
---------------------

SmartAnthill RNG provides a 'non-key Random Stream' for various purposes such as padding, ENTROPY data for the pairing (sic!), etc. Generation of 128 bits of non-key Random Stream is similar to key generation described above, with the following differences:

* instead of POORMAN4KEYS Poor-Man's PRNG, NONKEYPOORMAN Poor-Man's PRNG is used
* instead of AES(key=KEY4KEYS,data=DATA), DATA is used directly

References
----------

[Fortuna] Niels Ferguson, Bruce Schneier. "Practical Cryptography". Wiley Publishing, 2003. Sections 10.3 ('Fortuna') - 10.7 ('So What Should I Do?')

