#!/bin/bash
set -e
cd ${TRAVIS_BUILD_DIR}
git fetch --unshallow && git fetch --all --tags
if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
    mkdir -p ~/.config/QtProject/
    cp ${TRAVIS_BUILD_DIR}/test/qtlogging.ini ~/.config/QtProject/
elif [ "${TRAVIS_OS_NAME}" = "osx" ]; then
    mkdir -p ~/Library/Preferences/QtProject/
    cp ${TRAVIS_BUILD_DIR}/test/qtlogging.ini ~/Library/Preferences/QtProject/
elif [ "${TRAVIS_OS_NAME}" = "android" ]; then
    wget https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/gstreamer-1.0-android-armv7-1.5.2.tar.bz2
    mkdir -p ${TRAVIS_BUILD_DIR}/gstreamer-1.0-android-armv7-1.5.2
    tar jxf gstreamer-1.0-android-armv7-1.5.2.tar.bz2 -C ${TRAVIS_BUILD_DIR}/gstreamer-1.0-android-armv7-1.5.2
fi
