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

:Version:   v0.1.2

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

Devices with hardware-based entropy source
------------------------------------------

If Device doesn't have a pre-initialized Poor-Man's PRNG, it is known as Hardware-Entropy-Only-Based Device. For such Hardware-Entropy-Only-Based Devices the following applies:

* Device MUST still implement Poor-Man's PRNG, it is just not pre-initialized
* Device MUST have a hardware entropy source, which provides a hardware-generated bit stream
* Device MUST implement on-line testing of hardware-generated bit stream (monobit test, poker test, runs test, and long runs test, as they were specified in FIPS140-2 after Change Notice 1 and before Change Notice 2; testing should be performed on each 20000-bit block before this block is fed to Fortuna; if the test fails - another block MAY be taken; if the test fails on 3 blocks in a row - it is considered a hardware RNG failure), with a catastrophic Device failure if the test fails before at least required Poor-Man's RNGs are initialized
* on-line testing MUST be performed on a bit stream before any cryptographic primitives are applied (but SHOULD be performed after von Neumann bias removal)
* Device MUST implement Fortuna RNG (as specified in TODO; note that Fortuna is very resource-intensive for an MCU). 
* hardware-generated bit stream MUST be fed to a Fortuna PRNG (after 20000-bit blocks pass on-line testing)
* RNG MUST skip at least first TODO bits of the Fortuna output bit stream (before starting using Fortuna output), after each Device reset/reboot
* on Device start, after skipping required amount of Fortuna output as specified above, and after performing all the additional Entropy-Needed - Entropy-Provided exchanges as specified in :ref:`sapairing` document, Device SHOULD initialize Poor-Man's PRNG with Fortuna's output. As soon as Poor-Man's PRNGs are initialized:

  + regardless of hardware RNG failures, to obtain one byte of output bit stream, RNG MUST take one byte from Fortuna output, and XOR it with one byte of Poor-Man's PRNG output 
  + as long as hardware RNG doesn't fail (i.e. blocks do pass on-line testing as described above), Poor-Man's PRNG SHOULD be re-initialized approx. once per one hour of work (exact times MAY vary depending on typical Device patterns), by XOR-ing it's counter with next 128 bits of Fortuna output

In addition, such Devices SHOULD request extra entropy during pairing procedure, as described in :ref:`sapairing` document.

Devices with both pre-initialized Poor-Man's PRNG and hardware-based entropy source
-----------------------------------------------------------------------------------

If Device has both pre-initialized Poor-Man's PRNG and hardware-based entropy source (which is RECOMMENDED whenever feasible):

* such Devices are known as Unique-Hardware-Entropy-Based Devices
* implementing on-line testing is not necessary
* Device MUST implement Fortuna RNG
* hardware-generated bit stream MUST be fed to a Fortuna PRNG
* RNG MUST skip at least first TODO bits of the Fortuna output bit stream, after each Device reset/reboot
* to obtain one byte of output bit stream, RNG MUST take one byte from Fortuna output, and XOR it with one byte of Poor-Man's PRNG output

SmartAnthill Client (and Devices with Crypto-Safe RNG)
------------------------------------------------------

Even if the system where the SmartAnthill stack is running, has a supposedly crypto-safe RNG (such as built-in crypto-safe /dev/urandom), SmartAnthill implementations still MUST employ Poor-Man's PRNG (as described above) in addition to system-provided crypto-safe PRNG. In such cases, each byte of SmartAnthill RNG (which is provided to the rest of SmartAnthill) SHOULD be a XOR of 1 byte of system-provided crypto-safe PRNG, and 1 byte of Poor-Man's PRNG. 

*Rationale. This approach allows to reduce the impact of catastrophic failures of the system-provided crypto-safe PRNG (for example, it would mitigate effects of the Debian RNG disaster very significantly).*

To initialize Poor-Man's RNG on Client side, SmartAnthill implementation MUST NOT use the same crypto-safe RNG which output will be used for XOR-ing with Poor-Man's RNG (as specified above); instead, Poor-Man's RNG on Client side MUST be initialized independently; valid examples of such independent initialization include XOR-ing of at least two sources, such as an independent Fortuna RNG with user input (timing of typing or mouse movements), or online generators such as 'raw bytes' from random.org or from smartanthill.org (TODO); IMPORTANT: all exchanges with online generators MUST be over https, and with server certificate validation.

The same procedure SHOULD also be used for generating random data which is used for SmartAnthill key generation.

Key Generation
--------------

This sections describes rules for generating keys (and other key material, such as DH random numbers).

For Devices which support OtA Pairing (see :ref:`sapairing` document for details), key material needs to be generated. For such Devices the following requirements MUST be met:

* if Device doesn't have hardware-based entropy source:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.
  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - take 16 random bytes received from the Client side (see description of Pairing-Request in :ref:`sapairing` for details), as ENTROPY
    - calculate `output=AES(key=KEY4KEYS,AES(key=POORMAN4KEYS.Random16bytes(),data=ENTROPY))`

* if Device does have a hardware-based entropy source but doesn't have pre-initialized keys:

  + Device MUST implement at least Poor-Man's PRNGs (they're not pre-initialized; initialization is described below): one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used for other purposes. Both Poor-Man's PRNGs MUST be initialized right after Device first start, as described above
  + as described above, Device MUST implement Fortuna RNG (feeding hardware entropy to Fortuna as described above)
  + to generate 128 bits of key, the following procedure applies:

    - take 16 random bytes received from the Client side (see description of Pairing-Request in :ref:`sapairing` for details), as ENTROPY
    - if on-line hardware-based testing indicates that the hardware entropy is ok:

      * feed ENTROPY to Fortuna generator (the same instance of Fortuna as described above for such devices)
      * calculate `output=Fortuna.Random16Bytes()`

    - if on-line hardware-based testing indicates that the hardware entropy has failed:

      * calculate `output=AES(key=POORMAN4KEYS.Random16bytes(),data=ENTROPY)`

* if Device has both pre-initialized keys and hardware-based entropy source:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.
  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - take 16 random bytes received from the Client side (see description of Pairing-Request in :ref:`sapairing` for details), as ENTROPY
    - calculate `output=Fortuna.Random16bytes() XOR AES(key=KEY4KEYS,AES(key=POORMAN4KEYS.Random16bytes(),data=ENTROPY))`

* if Device (or Client) has a crypto-safe RNG:

  + Device MUST implement at least two pre-initialized Poor-Man's PRNGs: one of them (named 'POORMAN4KEYS') MUST NOT be used for any purposes except for key generation as described below. Another one (named 'NONKEYPOORMAN') is used to produce 'non-key Random Stream'.

    - Initialization of both Poor-Man's PRNGs (as well as initialization of KEY4KEYS and POORMAN4KEYS, see below) MUST be done independently, as specified in "SmartAnthill Client (and Devices with Crypto-Safe RNG)" section above.

  + in addition, Device MUST have an additional pre-initialized key (KEY4KEYS), which MUST NOT be used except for key generation as described below
  + to generate 128 bits of key, the following procedure applies:

    - take 16 random bytes received from the Client side (see description of Pairing-Request in :ref:`sapairing` for details), as ENTROPY
    - calculate `output=CryptoSafeRNG.Random16bytes() XOR AES(key=KEY4KEYS,AES(key=POORMAN4KEYS.Random16bytes(),data=ENTROPY))`

Non-Key Random Stream
---------------------

SmartAnthill RNG provides a 'non-key Random Stream' for various purposes such as padding, ENTROPY data for the pairing (sic!), etc. Generation of 128 bits of non-key Random Stream is similar to key generation described above, with the following differences:

* instead of POORMAN4KEYS Poor-Man's PRNG, for 'non-key Random Stream' NONKEYPOORMAN is used
* a 16-byte pre-defined block of data (for example, one may use macro containing something like `memset(ptr,0,16);*(uint16*)ptr=__LINE__;memcpy((char*)ptr+2,8,__TIME__);` to initialize such a block) is used instead of ENTROPY

