#!/bin/bash -x

if [ $# -ne 2 ]; then
	echo 'MakeQtTravisTarball.sh QtDirectory BuildType'
	exit 1
fi

QT_DIRECTORY=$1
if [ ! -d ${QT_DIRECTORY} ]; then
	echo 'Specify directory for Qt Directory to copy from.'
	exit 1
fi

QT_FULL_VERSION=5.12.6

QT_BUILD_TYPE=$2
if [ ! -d ${QT_DIRECTORY}/${QT_FULL_VERSION}/${QT_BUILD_TYPE} ]; then
        echo 'Qt build type directory not found. Specify example: clang_64 or ios'
    exit 1
fi

mkdir -p Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}/${QT_BUILD_TYPE}
cp -r ${QT_DIRECTORY}/${QT_FULL_VERSION}/${QT_BUILD_TYPE} Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}
rm -rf Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}/${QT_BUILD_TYPE}/doc
tar -jcvf Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}-min.tar.bz2 --exclude='*_debug*' Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}/

# Pull iOS GStreamer
if [ -d ~/Library/Developer/GStreamer/iPhone.sdk ]; then
    echo 'GStreamer for iOS found, archiving..'
	tar -jcvf gstreamer-ios-min.tar.bz2 -C ~/Library/Developer GStreamer/iPhone.sdk
fi

# Pull Mac OS GStreamer
if [ -d /Library/Frameworks/GStreamer ]; then
	echo 'GStreamer for MacOS found, archiving..'
	tar -jcvf gstreamer-macos-min.tar.bz2 /Library/Frameworks/GStreamer
fi
