#!/bin/bash
if [ ! -d /Volumes/RAMDisk ] ; then
    echo 'RAM Disk not found'
    echo 'Only used for App Store builds. It will not work on your computer.'
    exit 1
fi
#-- Set to my local installation
QMAKE=/Users/gus/Applications/Qt/5.12.2/ios/bin/qmake
#-- Using Travis variables as this will eventually live there
SHADOW_BUILD_DIR=/Volumes/RAMDisk/build-qgroundcontrol-iOS-Release
TRAVIS_BUILD_DIR=/Users/gus/github/work/UpstreamQGC
#-- Build it
mkdir -p ${SHADOW_BUILD_DIR} &&
cd ${SHADOW_BUILD_DIR} &&
#-- Create project only (build using Xcode)
${QMAKE} -r ${TRAVIS_BUILD_DIR}/qgroundcontrol.pro CONFIG+=WarningsAsErrorsOn CONFIG-=debug_and_release CONFIG+=release CONFIG+=ForAppStore
sed -i .bak 's/com.yourcompany.${PRODUCT_NAME:rfc1034identifier}/org.QGroundControl.qgc/' ${SHADOW_BUILD_DIR}/QGroundControl.xcodeproj/project.pbxproj
#-- Create and build
#${QMAKE} -r ${TRAVIS_BUILD_DIR}/qgroundcontrol.pro CONFIG+=WarningsAsErrorsOn CONFIG-=debug_and_release CONFIG+=release CONFIG+=ForAppStore &&
#xcodebuild -configuration Release -xcconfig ${TRAVIS_BUILD_DIR}/ios/qgroundcontrol_appstore.xcconfig
