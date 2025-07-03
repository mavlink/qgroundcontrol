# Resource Overrides

A "resource" in QGC source code terminology is anything found in Qt resources file:

* [qgroundcontrol.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgroundcontrol.qrc)
* [qgcresources.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgcresources.qrc)
* [InstrumentValueIcons.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/resources/InstrumenValueIcons/InstrumentValueIcons.qrc)
* Qml files

By overriding a resource you can replace it with your own version of it. This could be as simple as a single icon, or as complex as replacing an entire Vehicle Setup page of qml ui code. Be aware that using resource overrides means you are duplicating regular QGC source code. This can be a good or a bad thing depending on how you go about it. By using a resource override on a Qml file example it allows you to get away from dealing with crazy merge conflicts which you merge in newer versions of upstream QGC source. But it also means that you need to pay attention to what is going on in that upstream QGC source to see if you need to modify your copied code in a similar way.

Resource overrides work by using `QQmlEngine::addUrlInterceptor` to intercept requests for resources and re-route the request to available custom resources instead of the normal resource. Look at [custom_example/CustomPlugin.cc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/CustomPlugin.cc) for how it's done and replicate that in your own custom build source.

Custom resources that are meant for override are prepended with `/Custom` to the resource prefix in the custom resource file. The file alias for the resouce should be exactly the same as the normal QGC resource. Take a look at [custom_example/custom.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/custom.qrc) and [custom_example/CMakeLists.txt](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/CMakeLists.txt) for examples of how to do all of this.