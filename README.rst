SmartAnthill 2.0 Embedded System
================================

SmartAnthill project is intended to operate on MCUs which are extremely resource-constrained. In particular, currently we’re aiming to run SmartAnthill on MCUs with as little as 512 bytes of RAM. This makes the task of implementing SmartAnthill on such MCUs rather non-trivial. Present document aims to describe our approach to implementing SmartAnthill on MCU side. It should be noted that what is described here is merely our approach to a reference SmartAnthill implementation; it is not the only possible approach, and any other implementation which is compliant with SmartAnthill protocol specification, is welcome (as long as it can run on target MCUs and meet energy consumption and other requirements).

More detailed information is located in `Documentation <http://docs.smartanthill.org>`.
