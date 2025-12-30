# Resource Overrides

A "resource" in QGC source code terminology is anything found in Qt resources file:

* [qgcresources.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgcresources.qrc)
* [qgcimages.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgcimages.qrc)
* [InstrumentValueIcons.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/resources/InstrumentValueIcons/InstrumentValueIcons.qrc)
* Qml files

Overriding a resource allows you to replace it with your own version. This could be as simple as a single icon, or as complex as replacing an entire Vehicle Setup page of UI QML code. Note that using resource overrides means you are duplicating regular QGC source code. This can be a good or a bad thing, depending on how you go about it. By using a resource override on a QML file, for example, allows you to avoid dealing with merge conflicts when merging newer versions of the upstream QGC source. However, it also means that you need to pay attention to the latest version of the QGC source code to see if you need to modify your copied code in a similar way.

Resource overrides work by using `QQmlEngine::addUrlInterceptor` to intercept requests for resources and re-route the request to available custom resources instead of the standard ones. Look at [custom_example/src/CustomPlugin.cc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/src/CustomPlugin.cc) for how has been done, and replicate it in your own custom build source.

Custom resources, that are meant to override, have the prefix `/Custom` added to their resource name in the custom resource file. The file alias for the resource should be exactly the same as the standard QGC resource. For examples of how to do any of this, take a look at [custom_example/custom.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/custom.qrc) and [custom_example/CMakeLists.txt](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/CMakeLists.txt).
