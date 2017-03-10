#!/bin/sh

IOSDIR=/tmp/ios
OSXDIR=/tmp/$QT_DIR

# Uncomment below with the appropriate paths for your setup
# after installing the proper verion of qt.
# At least one of ios or osx is necessary.

#IOSDIR=/path/to/Qt/5.5/ios
#OSXDIR=/path/to/5.5/clang_64
#TRAVIS_BUILD_DIR=/path/to/qgroundcontrol

XCODEVER=`xcodebuild -version 2>&1 | (head -n1) | awk  '{print $2}'`
echo "Testing Xcode version: $XCODEVER"
MAJVER=${XCODEVER:0:1}
if [ X"$TRAVIS_BUILD_DIR" == "X" ]; then
    echo "Missing TRAVIS_BUILD_DIR configuration variable"
    exit 1
fi

if [ "$MAJVER" == "8" ]; then
    if [ -d $OSXDIR/bin ]; then
        QTVER=`$OSXDIR/bin/qmake -version | grep "Using Qt" | awk  '{print $4}'`
        echo "Testing Qt Version: $QTVER"
        if [ "$QTVER" == "5.5.1" ]; then
            echo "Found Xcode $XCODEVER for OSX. Applying patch:"
            cd $OSXDIR 2> /dev/null && \
            patch -N -p7 < ${TRAVIS_BUILD_DIR}/tools/qt_macos_xcode8.patch # 2>&1 > /dev/null
        fi
    fi
    if [ -d $IOSDIR/bin ]; then
        QTVER=`$IOSDIR/bin/qmake -version | grep "Using Qt" | awk  '{print $4}'`
        echo "Testing Qt Version: $QTVER"
        if [ "$QTVER" == "5.5.1" ]; then
            echo "Found Xcode $XCODEVER for iOS. Applying patch:"
            cd $IOSDIR 2> /dev/null && \
            patch -N -p7 < ${TRAVIS_BUILD_DIR}/tools/qt_ios_xcode8.patch # 2>&1 > /dev/null
        fi
    fi
fi
