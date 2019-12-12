# Building for non-linux platforms

Compiling the gstqmlgl plugin for non-linux platforms is not so trivial.
This file explains the steps that need to be followed for a successful build.

## Step 1

Build GStreamer for the target platform using cerbero.

## Step 2

Enter the cerbero shell:
```
./cerbero-uninstalled -c config/<target platform config>.cbc shell
```

## Step 3

Export the following environment variables:
```
export PATH=/path/to/Qt/<version>/<platform>/bin:$PATH
```

if you are cross-compiling (ex. for android), also export:
```
export PKG_CONFIG_SYSROOT_DIR=/
```

Additionally, if you are building for android:
```
export ANDROID_NDK_ROOT=$ANDROID_NDK
```

**Note**: the ANDROID_NDK variable is set by the cerbero shell; if you are not
using this shell, set it to the directory where you have installed the android
NDK. Additionally, if you are not building through the cerbero shell, it is also
important to have set PKG_CONFIG_LIBDIR to $GSTREAMER_ROOT/lib/pkgconfig.

## Step 4

cd to the directory of the gstqmlgl plugin and run:
```
qmake .
make
```

## Step 5

Copy the built plugin to your $GSTREAMER_ROOT/lib/gstreamer-1.0 or link to it
directly if it is compiled statically
