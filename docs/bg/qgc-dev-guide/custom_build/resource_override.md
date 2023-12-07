# Resource Overrides

A "resource" in QGC source code terminology is anything found in the [qgroundcontrol.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgroundcontrol.qrc) and [qgcresources.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/qgcresources.qrc) file. By overriding a resource you can replace it with your own version of it. This could be as simple as a single icon, or as complex as replacing an entire Vehicle Setup page of qml ui code.

Be aware that using resource overrides does not isolate you from upstream QGC changes like the plugin architecture does. In a sense you are directly modify the upstream QGC resources used by the main code.

## Exclusion Files

The first step to overriding a resource is to "exclude" it from the standard portion of the upstream build. This means that you are going to provide that resource in your own custom build resource file(s). There are two files which achieve this: qgroundcontrol.exclusion and [qgcresources.exclusion](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/qgcresources.exclusion). They correspond directly with the \*.qrc counterparts. In order to exclude a resource, copy the resource line from the .qrc file into the appropriate .exclusion file.

## Custom version of excluded resources

You must include the custom version of the overriden resouce in you custom build resource file. The resource alias must exactly match the upstream alias. The name and actual location of the resource can be anywhere within your custom directory structure.

## Generating the new modified versions of standard QGC resource file

This is done using the `updateqrc.py` python script. It will read the upstream `qgroundcontrol.qrc` and `qgcresources.qrc` file and the corresponding exclusion files and output new versions of these files in your custom directory. These new versions will not have the resources you specified to exclude in them. The build system for custom builds uses these generated files (if they exist) to build with instead of the upstream versions. The generated version of these file should be added to your repo. Also whenever you update the upstream portion of QGC in your custom repo you must re-run `python updateqrc.py` to generate new version of the files since the upstream resources may have changed.

## Custom Build Example

You can see an examples of custom build qgcresource overrides in the repo custom build example:

- [qgcresources.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/qgcresources.exclusion)
- [custom.qrc](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/custom.qrc)
