#!/usr/bin/env bash

# this requires `origin/master` in the git tree

MANIFEST_FILE=android/AndroidManifest.xml

VERSIONCODE=`git rev-list origin/master --first-parent --count`
VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`

# increment the versionCode past master for tagged releases
if [ "${TRAVIS_TAG}" ]; then
	let VERSIONCODE=${VERSIONCODE}+1
fi

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
