# Initial Repository Setup For Custom Build

- Navigate to the [QGC repo](https://github.com/mavlink/qgroundcontrol) and create your own fork.
- Copy the `custom-example` directory to a new `custom` directory at the root of the repo.
- Tweak the source in `custom` directory as needed.

You can also rename the `custom-example` directory to `custom` but that can lead to merge problems when you bring your fork up to date with newer upstream version of regular QGC.

When CMake finds a `custom` directory (containing a `CMakeLists.txt` and `cmake/CustomOverrides.cmake`) at the root of the repo, it builds that custom build instead of regular QGC; reconfigure from a clean build directory after adding it.
To build an overlay from another directory, configure with `-DQGC_CUSTOM_DIR=<dir>` (a path relative to the repo root). For example, CI builds the example in place with `-DQGC_CUSTOM_DIR=custom-example`.

## Modifying Mainline QGC Source Code

When creating your own custom build it is not uncommon to discover that you may need a tweak/addition to regular QGC source in places where the custom build architecture falls short. By discussing those needed changes firsthand with QGC devs and submitting pulls to make the custom build architecture better you make QGC more powerful for everyone and give back to the community.

It is best to keep modifications in mainline QGC source to a minimum. Since every change there may make it more difficult for you to keep up to date with merging in changes from regular QGC as it moves forward.
