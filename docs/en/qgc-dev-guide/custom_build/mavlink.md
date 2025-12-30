# MAVLink Customisation

QGC communicates with flight stacks using [MAVLink](https://mavlink.io/en/), a very lightweight messaging protocol that has been designed for the drone ecosystem.

QGC includes the [all.xml](https://mavlink.io/en/messages/all.html) dialect by default. The `all.xml` includes all other dialects in the [mavlink/mavlink](https://github.com/mavlink/mavlink/tree/master/message_definitions/v1.0) repository, and allows it to communicate with both PX4 and Ardupilot.
Previous versions of QGC (v4.2.8 and earlier), used the `ArduPilotMega.xml` dialect.

In order to add support for a new set of messages you should add them to [development.xml](https://mavlink.io/en/messages/development.html), [ArduPilotMega.xml](https://mavlink.io/en/messages/ardupilotmega.html), or [common.xml](https://mavlink.io/en/messages/common.html) (for PX4), or fork _QGroundControl_ and include your own dialect.

To modify the version of MAVLink used by QGC:

- Update the CMake MAVLink Options in [/qgroundcontrol/cmake/CustomOptions.cmake](https://github.com/mavlink/qgroundcontrol/tree/master/cmake/CustomOptions.cmake)
  or when using the built-in custom build support you can override these options in [/qgroundcontrol/custom/cmake/CustomOverrides.cmake](https://github.com/mavlink/qgroundcontrol/tree/master/custom-example/cmake/CustomOverrides.cmake).
  - QGC_MAVLINK_GIT_REPO - This is a link to the git repo to use, by default this is a link to https://github.com/mavlink/c_library_v2.
                           You can also [build your own libraries](https://mavlink.io/en/getting_started/generate_libraries.html) using the MAVLink toolchain and upload to your own git repo.
  - QGC_MAVLINK_GIT_TAG - This points to the git tag you would like to use in the chosen repo. This should likely be updated on occasion to use the latest version of MAVLink.

  - You can also set the mavlink directory to a local path by using the CMake variable CPM_mavlink_SOURCE.
  Just add to [/qgroundcontrol/cmake/CustomOptions.cmake](../../../../cmake/CustomOptions.cmake):
  ```cmake
  set(CPM_mavlink_SOURCE "/path/to/your/custom/mavlink")
  ```