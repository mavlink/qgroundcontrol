#!/usr/bin/env bash

VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`

# Android versionCode from git tag vX.Y.Z-123-gSHA
IFS=. read major minor patch dev sha <<<"${VERSIONNAME//-/.}"
VERSIONCODE=$(($major*100000))
VERSIONCODE=$(($(($minor*10000)) + $VERSIONCODE))
VERSIONCODE=$(($(($patch*1000)) + $VERSIONCODE))
VERSIONCODE=$(($(($dev)) + $VERSIONCODE))

# The 32 bit and 64 bit APKs each need there own version code.
if [ "$1" = "32" ]; then
    VERSIONCODE=330$VERSIONCODE
else
    VERSIONCODE=650$VERSIONCODE
fi

MANIFEST_FILE=android/AndroidManifest.xml

# manifest package
if [ "$2" = "master" ]; then
	QGC_PKG_NAME="org.mavlink.qgroundcontrolbeta"
	sed -i -e 's/package *= *"[^"]*"/package="'$QGC_PKG_NAME'"/' $MANIFEST_FILE
	echo "Android package name: $QGC_PKG_NAME"
fi

# android:versionCode
if [ -n "$VERSIONCODE" ]; then
	sed -i -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"$VERSIONCODE\"/" $MANIFEST_FILE
	echo "Android version: ${VERSIONCODE}"
else
	echo "Error versionCode empty"
	exit 0 # don't cause the build to fail
fi

# android:versionName
if [ -n "$VERSIONNAME" ]; then
	sed -i -e 's/versionName *= *"[^"]*"/versionName="'$VERSIONNAME'"/' $MANIFEST_FILE
	echo "Android name: ${VERSIONNAME}"
else
	echo "Error versionName empty"
	exit 0 # don't cause the build to fail
fi

