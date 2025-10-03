# Pull Requests

所有拉取请求都通过QGC CI构建系统，该系统构建版本和调试版本。 如果存在编译器警告，则构建将失败。 还针对支持的OS调试版本运行单元测试。 如果存在编译器警告，则构建将失败。 设备测试也是根据支持的操作系统调试构建运行的。

## Automatic Release Note Generation

Releases notes are generated from the following GitHub labels qhich should be set on Pull Requests as appropriate:

- "RN: MAJOR FEATURE"
- "RN: MINOR FEATURE"
- "RN: IMPROVEMENT"
- "RN: REFACTORING"
- "RN: BUGFIX"
- "RN: NEW BOARD SUPPORT"

There are also a set of the above labels which end in "- CUSTOM BUILD" which indicate the changes is associated with the custom build architecture.