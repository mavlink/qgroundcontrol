#!/bin/bash
if [ ! -d /Volumes/RAMDisk ] ; then
    echo 'RAM Disk not found'
    echo 'Only used for App Store builds. It will not work on your computer.'
    exit 1
fi
#-- Set to my local installation
QMAKE=/Users/gus/Applications/Qt/5.11.0/ios/bin/qmake
SHADOW_BUILD_DIR=/Volumes/RAMDisk/build-AuterionGS-iOS-Release
SOURCE_DIR=/Users/gus/github/Auterion/qgroundcontrol
#-- Build it
mkdir -p ${SHADOW_BUILD_DIR} &&
cd ${SHADOW_BUILD_DIR} &&
#-- Create project only (build using Xcode)
${QMAKE} -r ${SOURCE_DIR}/qgroundcontrol.pro CONFIG+=WarningsAsErrorsOn CONFIG-=debug_and_release CONFIG+=release CONFIG+=ForAppStore
sed -i .bak 's/com.yourcompany.${PRODUCT_NAME:rfc1034identifier}/com.auterion.auteriongs/' ${SHADOW_BUILD_DIR}/AuterionGS.xcodeproj/project.pbxproj
