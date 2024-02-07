# MAVLink Customisation

QGC communicates with flight stacks using [MAVLink](https://mavlink.io/en/), a very lightweight messaging protocol that has been designed for the drone ecosystem.

QGC includes the [all.xml](https://mavlink.io/en/messages/all.html) dialect by default. The `all.xml` includes all other dialects in the [mavlink/mavlink](https://github.com/mavlink/mavlink/tree/master/message_definitions/v1.0) repository, and allows it to communicate with both PX4 and Ardupilot.
Previous versions of QGC (v4.2.8 and earlier), used the `ArduPilotMega.xml` dialect.

In order to add support for a new set of messages you should add them to [development.xml](https://mavlink.io/en/messages/development.html), [ArduPilotMega.xml](https://mavlink.io/en/messages/ardupilotmega.html), or [common.xml](https://mavlink.io/en/messages/common.html) (for PX4), or fork _QGroundControl_ and include your own dialect.

To modify the version of MAVLink used by QGC:

- Replace the pre-build C library at [/qgroundcontrol/libs/mavlink/include/mavlink](https://github.com/mavlink/qgroundcontrol/tree/master/libs/mavlink/include/mavlink).
  - By default this is a submodule importing https\://github.com/mavlink/c_library_v2
  - You can change the submodule, or [build your own libraries](https://mavlink.io/en/getting_started/generate_libraries.html) using the MAVLink toolchain.
- You can change the whole dialect used by setting it in [`MAVLINK_CONF`](https://github.com/mavlink/qgroundcontrol/blob/master/QGCExternalLibs.pri#L52) when running _qmake_.
