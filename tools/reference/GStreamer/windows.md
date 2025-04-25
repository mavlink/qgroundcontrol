# Windows deployment

This page explains how to deploy GStreamer along your
application. There are different mechanisms, which have been reviewed
in [](deploying/index.md). The details for some of the
mechanisms are given here, and more options might be added to this
documentation in the future.

## Shared GStreamer

This is the easiest way to deploy GStreamer, although most of the time
it installs unnecessary files which grow the size of the installer and
the target drive free space requirements. Since GStreamer might be shared
among all applications that use it, though, the extra space requirements
are somewhat blurred.

Simply pack GStreamer  **runtime**  installer ([the same one you
installed in your development machine](installing/on-windows.md))
inside your installer (or download it from your installer) and execute
it silently using `msiexec`. `msiexec` is the tool that wraps most of
the Windows Installer functionality and offers a number of options to
suit your needs. You can review these options by
executing `msiexec` without parameters. For example using 1.8.1:

```
msiexec /i gstreamer-1.0-x86-1.8.1.msi
```

This will bring up the installation dialog as if the user had
double-clicked on the `msi` file. Usually, you will want to let the user
choose where they want to install GStreamer. An environment variable will
let your application locate it later on.

## Private deployment of GStreamer

You can use the same method as the shared GStreamer, but instruct its
installer to deploy to your application’s folder (or a
subfolder). Again, use the `msiexec` parameters that suit you best. For
example:

```
msiexec /passive INSTALLDIR=C:\Desired\Folder /i gstreamer-1.0-x86-1.8.1.msi
```

This will install GStreamer to `C:\Desired\Folder`  showing a progress
dialog, but not requiring user intervention.

## Deploy only necessary files, by manually picking them

On the other side of the spectrum, if you want to reduce the space
requirements (and installer size) to the maximum, you can manually
choose which GStreamer libraries to deploy. Unfortunately, you are on
your own on this road, besides using the [Dependency
Walker](http://www.dependencywalker.com/) tool to discover inter-DLL
dependencies.

Bear in mind that GStreamer is modular in nature. Plug-ins are loaded
depending on the media that is being played, so, if you do not know in
advance what files you are going to play, you do not know which DLLs you
need to deploy.

## Deploy only necessary packages, using provided Merge Modules

If you are building your installer using one of the Professional
editions of [Visual
Studio](http://www.microsoft.com/visualstudio/en-us/products/2010-editions/professional/overview)
or [WiX](http://wixtoolset.org) you can take advantage of pre-packaged
[Merge
Modules](http://msdn.microsoft.com/en-us/library/windows/desktop/aa369820\(v=vs.85\).aspx).
GStreamer is divided in packages, which roughly take care of
different tasks. There is the core package, the playback package, the
networking package, etc. Each package contains the necessary libraries
and files to accomplish its task.

The Merge Modules are pieces that can be put together to build a larger
Windows Installer. In this case, you just need to create a deployment
project for your application with Visual Studio and then add the Merge
Modules for the GStreamer packages your application needs.

This will produce a smaller installer than deploying the complete
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


If you include a merge module in your deployment project, remember to
include also its dependencies. Otherwise, the project will build
correctly and install flawlessly, but, when executing your application,
it will miss files.
