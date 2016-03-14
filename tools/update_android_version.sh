#!/usr/bin/env bash

# this requires `master` in the git tree
#  travis-ci branch builds are unable to set the version properly

MANIFEST_FILE=android/AndroidManifest.xml

VERSIONCODE=`git rev-list master --first-parent --count`
VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`

echo "VersionCode: ${VERSIONCODE}"
echo "VersionName: ${VERSIONNAME}"

if [ -n "$VERSIONCODE" ]; then
	sed -i -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"$VERSIONCODE\"/" $MANIFEST_FILE
	echo "Android version: ${VERSIONCODE}"
else
	echo "Error versionCode empty"
	exit 0 # don't cause the build to fail
fi

if [ -n "$VERSIONNAME" ]; then
	sed -i -e 's/versionName *= *"[^"]*"/versionName="'$VERSIONNAME'"/' $MANIFEST_FILE
	echo "Android name: ${VERSIONNAME}"
else
	echo "Error versionName empty"
	exit 0 # don't cause the build to fail
fi
