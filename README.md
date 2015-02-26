# grfutils

grfutils consists of a library and tools to communicate with GIRA smoke detectors via a radio module.
It was tested with a "GIRA smoke alarm device Dual" in combination with a GIRA diagnosis device.

Currently grfutils allow to 

* scan for groups of smoke detectors
* scan for smoke detector devices within a group
* read the properties of a device including temperature, battery state, etc.

Not yet implemented features are

* activating the accustic signal of a device
* deactivating the accustic signal of a device

## KUDOS

Many thanks go to the Selfbus project (http://www.selfbus.org) for providing a starting point to
understand the protocol between the PC and the diagnosis device.

## HELP NEEDED

While many aspects of the protocol from and to the diagnosis device are known, some dark areas remain.
If you have fun in reverse engineering unknown register content or communication between the
smoke detector device and the radio module, please take a look and provide your findings!  

