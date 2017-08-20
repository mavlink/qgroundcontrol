#!/usr/bin/env bash

major=1
minor=0
patch=$(git --git-dir .git rev-list master --first-parent --count)

VERSIONCODE=$(($major*100000000))
VERSIONCODE=$(($(($minor*100000)) + $VERSIONCODE))
VERSIONCODE=$(($(($patch)) + $VERSIONCODE))

VERSIONNAME="$major.$minor.$patch"

MANIFEST_FILE=custom/android_typhoonh/AndroidManifest.xml

# set manifest package name
QGC_PKG_NAME="com.yuneec.datapilot"
sed -i -e 's/package *= *"[^"]*"/package="'$QGC_PKG_NAME'"/' $MANIFEST_FILE
echo "Android package name: $QGC_PKG_NAME"

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

