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
    DAMAGE SUCH DAMAGE

.. _sazeptopatterns:

Zepto Programming Patterns
==========================

*NB: this document relies on certain terms and concepts introduced in* :ref:`saoverarch` *, * :ref:`sazeptovm` *, and* :ref:`saplugin` *documents, please make sure to read them before proceeding.*

Zepto VM does not intend to provide highly sophisticated and/or mathematics-oriented functionality; instead, it intends to support very limited but highly practical scenarios, which allow to minimize communications between SmartAnthill Central Controller and SmartAnthill Device, therefore allowing to minimize power consumption on the side of SmartAnthill Device. 

Currently, Zepto programming patterns are described in terms of Zepto Lua and Zepto Python. These two are extremely limited versions of their normal counterparts ("zepto" means "10^-21", so you get about 10^-21 functionality out of original languages). Basically, whatever is not in these examples, you shouldn't use. 

Pattern 1. Simple Request/Response
----------------------------------

The very primitive (though, when aided with a program on Central Controller side, sufficient to implement any kind of logic) program:

Zepto Lua:

.. code-block:: lua

  return TemperatureSensor.Execute()

Zepto Python:

.. code-block:: python

  return TemperatureSensor.Execute()

This program should compile to all Zepto VM Levels.

Pattern 2. Sleep and Measure
----------------------------

Sleep for several minutes (turning off transmitter), then report back. 

Zepto Lua:

.. code-block:: lua

  mcu_sleep(5*60) --5*60 is a compile-time constant, 
                  --  so no multiplication is performed on MCU here
  return TemperatureSensor.Execute()

Zepto Python:

.. code-block:: python

  mcu_sleep(5*60)
  return TemperatureSensor.Execute()

This program should compile to all Zepto VM Levels.

Pattern 3. Measure and Report If
--------------------------------

The same thing, but asking to report only if measurements exceed certain bounds. Still, once per 5 cycles, SmartAnthill Device reports back, so that Central Controller knows that the Device is still alive.

Zepto Lua:

.. code-block:: lua

  for i=1,5 do
    temp = TemperatureSensor.Execute()
    if temp.Temperature < 36.0 
       or temp.Temperature > 38.9 then --Note that both comparisons should compile 
                                       --  into integer comparisons, using Plugin Manifest
      return temp
    end
    mcu_sleep(5*60)
  end
  return TemperatureSensor.Execute()

Zepto Python:

.. code-block:: python

  for i in range(1,5):
    temp = TemperatureSensor.Execute()
    if temp.Temperature < 36.0
       or temp.Temperature > 38.9:
       return temp
    mcu_sleep(5*60)
  return TemperatureSensor.Execute()

This program should compile to all Zepto VM Levels, starting from Zepto VM Small.

Pattern 4. Implicit parallelism
-------------------------------

Zepto Lua:

.. code-block:: lua

  temp = TemperatureSensor.Execute()
  humi = HumiditySensor.Execute()
  return temp, humi

or 

.. code-block:: lua

  return TemperatureSensor.Execute(), HumiditySensor.Execute()

Zepto Python:

.. code-block:: python

  temp = TemperatureSensor.Execute()
  humi = HumiditySensor.Execute()
  return (temp, humi)

or

.. code-block:: python

  return (TemperatureSensor.Execute(),HumiditySensor.Execute())

In all these (equivalent) cases compiler, if possible, SHOULD implicitly call both sensor Execute() functions in parallel (see PARALLEL Zepto VM instruction), reducing processing time. 

Combined Example
----------------

Now let's consider an example where we want to perform temperature measurements more frequently than humidity ones, and 

Zepto Lua:

.. code-block:: lua

  humi = HumiditySensor.Execute()
  for i=1,5 do
    if(i%2 == 0) -- should compile into "&1"
      humi = HumiditySensor.Execute()
    temp = TemperatureSensor.Execute() -- SHOULD be performed in parallel
                                       -- with HumiditySensor() when applicable
    if humi.HumiditySensor > 80 and
       temp.Temperature > 30.0 then
      return temp, humi
    end
    mcu_sleep(5*60)
  end
  return TemperatureSensor.Execute(), HumiditySensor.Execute()

.. code-block:: python

  humi = HumiditySensor.Execute()
  for i in range(1,5):
    if i%2 == 0:
      humi = HumiditySensor.Execute()
    temp = TemperatureSensor.Execute()
    if humi.HumiditySensor > 80 and
       temp.Temperature > 30.0:
      return temp, humi
    end
    mcu_sleep(5*60)
  end
  return (TemperatureSensor.Execute(), HumiditySensor.Execute())

TODO: calculation plugins(?)

