# QGroundControl Ground Control Station

## Custom Build Example

To build this sample custom version, simply rename the directory from `custom-example` to `custom` before running `qmake` (or launching Qt Creator.) The build system will automatically find anything in `custom` and incorporate it into the build. If you had already run a build before renaming the directory, delete the build directory before running `qmake`. To restore the build to a stock QGroundControl one, rename the directory back to `custom-example` (making sure to clean the build directory again.)

### Custom Builds

The root project file (`qgroundcontrol.pro`) will look and see if `custom/custom.pri` exists. If it does, it will load it before anything else is setup. This allows you to modify the build in any way necessary for a custom build. This example shows you how to:

* Fully brand your build
* Define a single flight stack to avoid carrying over unnecessary code
* Implement your own, autopilot and firmware plugin overrides
* Implement your own camera manager and plugin overrides
* Implement your own QtQuick interface module
* Implement your own toolbar, toolbar indicators and UI navigation
* Implement your own Fly View overlay (and how to hide elements from QGC such as the flight widget)
* Implement your own, custom QtQuick camera control
* Implement your own, custom Pre-flight Checklist
* Define your own resources for all of the above

Note that within `qgroundcontrol.pro`, most main build steps are surrounded by flags, which you can define to override them. For example, if you want to have your own Android build, done in some completely different way, you simply:

```
DEFINES += DISABLE_BUILTIN_ANDROID
```

With this defined within your `custom.pri` file, it is up to you to define how to do the Android build. You can either replace the entire process or prepare it before invoking QGC’s own Android project file on your own. You would do this if you want to have your own branding within the Android manifest. The same applies to iOS (`DISABLE_BUILTIN_IOS`).
