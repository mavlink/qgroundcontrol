# Custom Build Plugins

The mechanisms for customizing QGC for a custom build is through the existing `FirmwarePlugin`, `AutoPilotPlugin` and `QGCCorePlugin` architectures. By creating subclasses of these plugins in your custom build you can change the behavior of QGC to suit your needs without needed to modify the upstream code.

## QGCCorePlugin

This allows you to modify the parts of QGC which are not directly related to vehicle but are related to the QGC application itself. 这包括诸如应用程序设置、调色板、品牌标识等等内容。
