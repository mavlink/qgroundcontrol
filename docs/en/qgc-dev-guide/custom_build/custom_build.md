# Custom Builds

A custom build allows third parties to create their own version of QGC in a way that allows them to easily keep up to date with changes which are being made in regular QGC. QGC has an architecture built into it which allows custom builds to modify and add to the feature set of regular QGC.

Some possibilities with a custom build

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

One of the downsides of QGC providing both generic support for any vehicle which supports mavlink as well as providing firmware specific support for both PX4 Pro and ArduPilot is complexity of the user interface. Since QGC doesn't know any information about your vehicle ahead of time it requires UI bits which can get in the way if the vehicle you fly only uses PX4 Pro firmware and is a multi-rotor vehicle. If that is a known thing then the UI can be simplified in various places. Also QGC targets both DIY users who are building their own vehicles from scratch as well as commercial users of off the shelf vehicles. Setting up a DIY drone from scratch requires all sort of functionality which is not needed for users of off the shelf vehicles. So for off the shelf vehicle users all the DIY specific stuff is just extra noise they need to look past. Creating a custom build allows you to specify exact details for your vehicle and hide things which are irrelevant thus creating an even simple user experience for your users than regular generic QGC.

There is a plugin architecture in QGC which allows for this custom build creation. They can be found in QGCCorePlugin.h, FirmwarePlugin.h and AutoPilotPlugin.h associated classes. To create a custom build you create subclasses of the standard plugins overriding the set of methods which are appropriate for you usage. 

There is also a mechanism which allows you to override resources so you can change the smaller visual elements in QGC.

Also internal to QGC is the concept of an "Advanced Mode". Whereas a standard QGC builds always runs in advanced mode. A custom build always starts out in regular/not advanced mode. There is an easier mechanism in the build to turn on advanced mode which is to click the fly view button 5 times in a row fairly quickly. If you do this in a custom build you will be warned about entering advanced mode. The concept here is to hide things which normal users should not have access to behind advanced mode. For example a commercial vehicle will not need access to most setup pages which are oriented to DIY setup. So a custom build can hide this. The custom example code shows how to do this.

If you want to understand the possibilities, the first step is to read through those files which document what is possible. Next look through the [```custom-example```](https://github.com/mavlink/qgroundcontrol/tree/master/custom-example) source code including the [README](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/README.md).