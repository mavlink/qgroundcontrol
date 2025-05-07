# Mac OS X deployment

This page explains how to deploy GStreamer along your application. There
are different mechanisms, which have been reviewed in [](deploying/index.md). The details for some
of the mechanisms are given here, and more options might be added to
this documentation in the future.

**FIXME: PackageMaker is dead we need a new solution **

## Shared GStreamer

This is the easiest way to deploy GStreamer, although most of the time
it installs unnecessary files which grow the size of the installer and
the target drive free space requirements. Since GStreamer might be shared
among all applications that use it, though, the extra space requirements
are somewhat blurred.

With PackageMaker, simply add GStreamer  **runtime ** disk image
 ([the same one you used to install the runtime in your development
machine](installing/on-mac-osx.md)) inside your installer
package and create a post-install script that mounts the disk image and
installs GStreamer package. You can use the following example, where you
should replace `$INSTALL_PATH` with the path where your installer copied
GStreamer's disk image files (the `/tmp` directory is good place to
install it as it will be removed at the end of the installation):

``` bash
hdiutil attach $INSTALL_PATH/gstreamer-1.0-1.8.1-x86_64-packages.dmg
cd /Volumes/gstreamer-1.0-1.8.1-x86_64/
installer -pkg gstreamer-1.0-1.8.1-x86_64.pkg -target "/"
hdiutil detach /Volumes/gstreamer-1.0-1.8.1-x86_64/
rm $INSTALL_PATH/gstreamer-1.0-1.8.1-x86_64-packages.dmg
```

## Private deployment of GStreamer

You can decide to distribute a private copy of GStreamer with your
application, although it's not the recommended method. In this case,
simply copy the framework to the application's Frameworks folder as
defined in the [bundle programming
guide](https://developer.apple.com/library/mac/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW19):

``` bash
cp -r /Library/Frameworks/GStreamer.framework ~/MyApp.app/Contents/Frameworks
```

Note that you can have several versions of GStreamer, and targeting
different architectures, installed in the system. Make sure you only
copy the version you need and that you update accordingly the link
`GStreamer.framework/Version/Current`:

``` bash
$ ls -l Frameworks/GStreamer.framework/Version/Current
lrwxr-xr-x 1 fluendo staff 21 Jun 5 18:46 Frameworks/GStreamer.framework/Versions/Current -> ../Versions/0.10/x86
```

Since GStreamer will be relocated, you will need to follow the
instructions on how to relocate GStreamer at the end of this page.

## Deploy only necessary files, by manually picking them

On the other side of the spectrum, if you want to reduce the space
requirements (and installer size) to the maximum, you can manually
choose which GStreamer libraries to deploy. Unfortunately, you are on
your own on this road, besides using the object file displaying tool:
[otool](https://developer.apple.com/library/mac/#documentation/darwin/reference/manpages/man1/otool.1.html).
Being a similar technique to deploying a private copy of GStreamer, keep
in mind that you should relocate GStreamer too, as explained at the end of
this page.

Bear also in mind that GStreamer is modular in nature. Plug-ins are
loaded depending on the media that is being played, so, if you do not
know in advance what files you are going to play, you do not know which
plugins and shared libraries you need to deploy.

## Deploy only necessary packages, using the provided ones

This will produce a smaller installer than deploying complete
GStreamer, without the added burden of having to manually pick each
library. You just need to know which packages your application requires.

| Package name | Dependencies | Licenses | Description |
|--------------|--------------|----------|-------------|
| base-system-1.0  | |JPEG, FreeType, BSD-like, LGPL, LGPL-2+, LGPL-2.1, LibPNG and MIT | Base system dependencies |
| gstreamer-1.0-capture | gstreamer-1.0-core, gstreamer-1.0-encoding | LGPL and LGPL-2+ | GStreamer plugins for capture |
| gstreamer-1.0-codecs | base-crypto, gstreamer-1.0-core | BSD, Jasper-2.0, BSD-like, LGPL, LGPL-2, LGPL-2+, LGPL-2.1 and LGPL-2.1+ | GStreamer codecs |
| gstreamer-1.0-codecs-gpl | gstreamer-1.0-core | BSD-like, LGPL, LGPL-2+ and LGPL-2.1+ | GStreamer codecs under the GPL license and/or with patents issues |
| gstreamer-1.0-core | base-system-1.0 | LGPL and LGPL-2+ | GStreamer core |
| gstreamer-1.0-dvd | gstreamer-1.0-core | GPL-2+, LGPL and LGPL-2+ | GStreamer DVD support |
| gstreamer-1.0-effects | gstreamer-1.0-core | LGPL and LGPL-2+ | GStreamer effects and instrumentation plugins |
| gstreamer-1.0-net | base-crypto, gstreamer-1.0-core | GPL-3, LGPL, LGPL-2+, LGPL-2.1+ and LGPL-3+ | GStreamer plugins for network protocols |
| gstreamer-1.0-playback | gstreamer-1.0-core | LGPL and LGPL-2+ | GStreamer plugins for playback |
| gstreamer-1.0-system | gstreamer-1.0-core | LGPL, LGPL-2+ and LGPL-2.1+ | GStreamer system plugins |
| gstreamer-1.0-visualizers | gstreamer-1.0-core | LGPL and LGPL-2+ | GStreamer visualization plugins |
| gstreamer-1.0-encoding | gstreamer-1.0-core, gstreamer-1.0-playback | LGPL and LGPL2+ | GStreamer plugins for encoding |
| gstreamer-1.0-editing | gstreamer-1.0-core, gstreamer-1.0-devtools | LGPL and LGPL2+ | GStreamer libraries and plugins for non linear editing |
| gstreamer-1.0-devtools | gstreamer-1.0-core | LGPL and LGPL2+ | GStreamer developers tools |
| gstreamer-1.0-libav | gstreamer-1.0-core | LGPL and LGPL2+ | GStreamer plugins wrapping ffmpeg |
| gstreamer-1.0-net-restricted | base-crypto, gstreamer-1.0-core | LGPL and LGPL2+ | GStreamer plugins for network protocols with potential patent issues in some countries |
| gstreamer-1.0-codecs-restricted | gstreamer-1.0-core | LGPL and LGPL2+ | GStreamer restricted codecs with potential patent issues in some countries |
| base-crypto | base-system-1.0 | LGPL and LGPL2+ | Cryptographic libraries |



## Relocation of GStreamer in OS X

In some situations we might need to relocate GStreamer, moving it to a
different place in the file system, like for instance when we are
shipping a private copy of GStreamer with our application.

### Location of dependent dynamic libraries.

On Darwin operating systems, the dynamic linker doesn't locate dependent
dynamic libraries using their leaf name, but instead it uses full paths,
which makes it harder to relocate them as explained in the DYNAMIC
LIBRARY LOADING section of
[dyld](https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man1/dyld.1.html)'s
man page:

> Unlike many other operating systems, Darwin does not locate dependent
> dynamic libraries via their leaf file name. Instead the full path to
> each dylib is used (e.g. /usr/lib/libSystem.B.dylib). But there are
> times when a full path is not appropriate; for instance, may want your
> binaries to be installable in anywhere on the disk.

We can get the list of paths used by an object file to locate its
dependent dynamic libraries
using [otool](https://developer.apple.com/library/mac/#documentation/darwin/reference/manpages/man1/otool.1.html):

``` bash
$ otool -L /Library/Frameworks/GStreamer.framework/Commands/gst-launch-1.0
/Library/Frameworks/GStreamer.framework/Commands/gst-launch-1.0:
 /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 550.43.0)
 /Library/Frameworks/GStreamer.framework/Versions/0.10/x86/lib/libgstreamer-1.0.0.dylib (compatibility version 31.0.0, current version 31.0.0)
 /Library/Frameworks/GStreamer.framework/Versions/0.10/x86/lib/libxml2.2.dylib (compatibility version 10.0.0, current version 10.8.0)
...
```

As you might have already noticed, if we move GStreamer to a different
folder, it will stop working because the runtime linker won't be able to
find `gstreamer-1.0` in the previous location
`/Library/Frameworks/GStreamer.framework/Versions/0.10/x86/lib/libgstreamer-1.0.0.dylib`.

This full path is extracted from the dynamic library  ***install name***
, a path that is used by the linker to determine its location. The
install name of a library can be retrieved with
[otool](https://developer.apple.com/library/mac/#documentation/darwin/reference/manpages/man1/otool.1.html) too:

``` bash
$ otool -D /Library/Frameworks/GStreamer.framework/Libraries/libgstreamer-1.0.dylib
/Library/Frameworks/GStreamer.framework/Libraries/libgstreamer-1.0.dylib:
/Library/Frameworks/GStreamer.framework/Versions/0.10/x86/lib/libgstreamer-1.0.0.dylib
```

Any object file that links to the dynamic library `gstreamer-1.0` will
use the
path `/Library/Frameworks/GStreamer.framework/Versions/0.10/x86/lib/libgstreamer-1.0.0.dylib` to
locate it, as we saw previously with `gst-launch-1.0`.

Since working exclusively with full paths wouldn't let us install our
binaries anywhere in the path, the linker provides a mechanism of string
substitution, adding three variables that can be used as a path prefix.
At runtime the linker will replace them with the generated path for the
prefix. These variables are `@executable_path`,
`@loader_path` and `@rpath`, described in depth in the DYNAMIC LIBRARY
LOADING section
of [dyld](https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man1/dyld.1.html)'s
man page.

For our purpose we will use the `@executable_path` variable, which is
replaced with a fixed path, the path to the directory containing the
main executable: `/Applications/MyApp.app/Contents/MacOS`.
The `@loader_path` variable can't be used in our scope, because it will
be replaced with the path to the directory containing the mach-o binary
that loaded the dynamic library, which can vary.

Therefore, in order to relocate GStreamer we will need to replace all
paths
containing `/Library/Frameworks/GStreamer.framework/` with `@executable_path/../Frameworks/GStreamer.framework/`, which
can be done using
the [install\_name\_tool](http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man1/install_name_tool.1.html)
utility

### Relocation of the binaries

As mentioned in the previous section, we can use
the `install_name_tool` in combination with `otool` to list all paths
for dependant dynamic libraries and modify them to use the new location.
However GStreamer has a huge list of binaries and doing it manually would
be a painful task. That's why a simple relocation script is provided
which you can find in cerbero's repository
(`cerbero/tools/osxrelocator.py`). This scripts takes 3 parameters:

1.  `directory`: the directory to parse looking for binaries
2.  `old_prefix`: the old prefix we want to change (eg:
    `/Library/Frameworks/GStreamer.framework`)
3.  `new_prefix`: the new prefix we want to use
    (eg: `@executable_path/../Frameworks/GStreamer.framework/`)

When looking for binaries to fix, we will run the script in the
following
directories:

``` bash
$ osxrelocator.py MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
$ osxrelocator.py MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
$ osxrelocator.py MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/bin /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
$ osxrelocator.py MyApp.app/Contents/MacOS /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
```

### Adjusting environment variables with the new paths

The application also needs to set the following environment variables to
help other libraries finding resources in the new
    path:

  - `GST_PLUGIN_SYSTEM_PATH=/Applications/MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0`
  - `GST_PLUGIN_SCANNER=/Applications/MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner`
  - `GTK_PATH=/Applications/MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/`
  - `GIO_EXTRA_MODULES=/Applications/MyApp.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules`

You can use the following functions:

  - C: [putenv("VAR=/foo/bar")](http://linux.die.net/man/3/putenv)

  - Python: [os.environ\['VAR'\] =
    '/foo/var'](http://docs.python.org/library/os.html)
