# Plugin Architecture

Although the MAVLink spec defines a standard communication protocol to communicate with a vehicle. There are many aspects of that spec that are up for interpretation by the firmware developers. Because of this there are many cases where communication with a vehicle running one firmware is be slightly different than communication with a vehicle running a different firmware in order to accomplish the same task. Also each firmware may implement a different subset of the MAVLink command set.

Another major issue is that the MAVLink spec does not cover vehicle configuration or a common parameter set. Due to this all code which relates to vehicle setup ends up being firmware specific. Also any code which must refer to a specific parameter is also firmware specific.

Given all of these differences between firmware implementations it can be quite tricky to create a single ground station application that can support each without having the codebase degrade into a massive pile of if/then/else statements peppered everywhere based on the firmware the vehicle is using.

QGC uses a plugin architecture to isolate the firmware specific code from the code which is generic to all firmwares. There are two main plugins which accomplish this ```FirmwarePlugin``` and ```AutoPilotPlugin```.

This plugin architecture is also used by custom builds to allow ever further customization beyond when standard QGC can provide.

## FirmwarePlugin

This is used to create a standard interface to parts of Mavlink which are generally not standardized.

## AutoPilotPlugin

This is used to provide the user interface for Vehicle Setup.

## QGCCorePlugin

This is used to expose features of the QGC application itself which are not related to a Vehicle through a standard interface. This is then used by custom builds to adjust the QGC feature set to their needs.