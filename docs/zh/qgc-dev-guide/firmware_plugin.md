# Plugin Architecture

虽然MAVLink规范定义了与载具通信的标准通信协议。 There are many aspects of that spec that are up for interpretation by the firmware developers. Because of this there are many cases where communication with a vehicle running one firmware is be slightly different than communication with a vehicle running a different firmware in order to accomplish the same task. Also each firmware may implement a different subset of the MAVLink command set.

另一个主要问题是MAVLink规范不包括载具配置或通用参数集。 Due to this all code which relates to vehicle setup ends up being firmware specific. 此外，任何必须引用特定参数的代码也是特定于固件的。

鉴于固件实现之间的所有这些差异，创建单个地面站应用程序可能非常棘手，可以支持每个应用程序而不会使代码库降级为基于车辆使用的固件在任何地方遍布的大量if / then / else语句。

QGC uses a plugin architecture to isolate the firmware specific code from the code which is generic to all firmwares. There are two main plugins which accomplish this ```FirmwarePlugin``` and ```AutoPilotPlugin```.

This plugin architecture is also used by custom builds to allow ever further customization beyond when standard QGC can provide.

## FirmwarePlugin

This is used to create a standard interface to parts of Mavlink which are generally not standardized.

## AutoPilotPlugin

This is used to provide the user interface for Vehicle Setup.

## QGCCorePlugin

This is used to expose features of the QGC application itself which are not related to a Vehicle through a standard interface. This is then used by custom builds to adjust the QGC feature set to their needs.