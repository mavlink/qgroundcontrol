# Resource Overrides

A "resource" in QGC source code terminology is anything declared in Qt's resource system,
including:

- [Root resource declarations](https://github.com/mavlink/qgroundcontrol/blob/master/CMakeLists.txt)
- Per-module resources declared via `qt_add_resources` in the relevant `CMakeLists.txt` (e.g., under `src/`)
- QML files

Overriding a resource allows you to replace it with your own version. This could be as simple as a single icon, or as complex as replacing an entire Vehicle Setup page of UI QML code. Note that using resource overrides means you are duplicating regular QGC source code. This can be a good or a bad thing, depending on how you go about it. By using a resource override on a QML file, for example, allows you to avoid dealing with merge conflicts when merging newer versions of the upstream QGC source. However, it also means that you need to pay attention to the latest version of the QGC source code to see if you need to modify your copied code in a similar way.

Resource overrides use `QQmlEngine::addUrlInterceptor` to intercept requests and route them to available custom resources instead of the standard ones. See [custom-example/src/CustomPlugin.cc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/src/CustomPlugin.cc) for an example you can adapt to your own custom build.

Custom resources intended as overrides have the prefix `/Custom` added to their resource name. The file alias should exactly match the standard QGC resource. See the [`qt_add_resources` declarations in custom-example/CMakeLists.txt](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/CMakeLists.txt) for examples.
