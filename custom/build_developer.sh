#!/bin/bash

# Note
#
# There is a (Java) bug that once in a while a build will fail with something like this below.
# When this happens, just run the build again.
#
# FAILURE: Build failed with an exception.
# 
# * What went wrong:
# Execution failed for task ':mergeDebugResources'.
# > Crunching Cruncher icon.png failed, see logs

die() {
    echo $1
    exit -1    
}

unameOut=`uname -s`
QTVERSION=5.9.1

#-- Adjust these to your Android SDK/NDK and Qt installation
if [ "$unameOut" = "Linux" ]; then
    export ANDROID_NDK_ROOT=~/dpbuild/Android/Sdk/ndk-bundle
    export ANDROID_SDK_ROOT=~/dpbuild/Android/Sdk
    QTINSTALL=~/dpbuild/qt/$QTVERSION
else
    export ANDROID_NDK_ROOT=/Users/gus/Library/Android/sdk/ndk-bundle
    export ANDROID_SDK_ROOT=/Users/gus/Library/Android/sdk
    QTINSTALL=/Users/gus/Applications/Qt/$QTVERSION
fi

[ -d $ANDROID_NDK_ROOT ] || die "Could not locate ANDROID_NDK_ROOT"
[ -d $ANDROID_SDK_ROOT ] || die "Could not locate ANDROID_SDK_ROOT"
[ -d $QTINSTALL ] || die "Could not locate Qt $QTVERSION installation"
[ -e $QTINSTALL/android_x86/bin/qmake ] || die "Could not locate Qt $QTVERSION installation for Android x86"

#-- The build will run in the directory below
BUILDDIR=/tmp/datapilot_build
[ -d $BUILDDIR ] || mkdir -p $BUILDDIR

#-- No need to change anything below unless you are customizing it

clean() {
    [ -d $BUILDDIR ] && rm -rf $BUILDDIR/* && rm -rf $BUILDDIR/.*
}

build() {
    cd $BUILDDIR && $QTINSTALL/android_x86/bin/qmake $ROOTDIR/../qgroundcontrol.pro -spec android-g++ CONFIG+=DeveloperBuild && make -j$CORES 2>&1 || exit 1 | tee build.log
    echo
    echo "Package in $BUILDDIR/release/package/DataPilotDevel.apk"
    echo "Build log in $BUILDDIR/build.log"
    echo
    exit 0
}

case "${unameOut}" in
    Linux*)     CORES=`cat /proc/cpuinfo | awk '/^processor/{print $3}' | tail -1`;;
    Darwin*)    CORES=`sysctl -n hw.ncpu`;;
    *)          die "Unknwon platform: ${unameOut}"
esac
case $0 in /*) SCRIPT="$0";; *) SCRIPT="`pwd`/${0#./}";; esac
ROOTDIR=`dirname "$SCRIPT"`
SCRIPT=`basename "$SCRIPT"`
[ -d $BUILDDIR ] || mkdir $BUILDDIR || die "Error creating $BUILDDIR"
case "$1" in
    clean)
        clean
        exit 0
        ;;
    rebuild)
        clean
        build
        ;;
    build)
        build
        ;;
    *)
        echo $"Usage: $0 {clean|build|rebuild}"
        exit 1
        ;;
esac
