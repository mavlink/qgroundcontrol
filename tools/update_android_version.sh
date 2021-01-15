#!/usr/bin/env bash

# git tag is in the form vX.Y.Z for builds from tagged commit 
# git tag is in the form vX.Y.Z-1234-gSHA for builds from non-tagged commit 

# Strip the 'v' from the beginning of the tag
VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`
echo "tag $VERSIONNAME"

# Change all occurences of '-' in tag to '.' and separate into parts
IFS=. read major minor patch dev sha <<<"${VERSIONNAME//-/.}"
echo "major:$major minor:$minor patch:$patch dev:$dev sha:$sha"

# Max Android version code is 2100000000. Version codes must increase with each release and the 
# version codes for multiple apks for the same release must be unique and not collide as well. 
# All of this makes it next to impossible to create a rational system of building a version code
# from a semantic version without imposing some strict restrictions.
if [ $major -gt 9 ]; then
    echo "Error: Major version larger than 1 digit: $major"
    exit 1
fi
if [ $minor -gt 9 ]; then
    echo "Error: Minor version larger than 1 digit: $minor"
    exit 1
fi
if [ $patch -gt 99 ]; then
    echo "Error: Patch version larger than 2 digits: $patch"
    exit 1
fi
if [ $dev -gt 999 ]; then
    echo "Error: Dev version larger than 3 digits: $dev"
    exit 1
fi

# Version code format: BBMIPPDDD (B=Bitness, I=Minor)
VERSIONCODE=$(($major*1000000))
VERSIONCODE=$(($(($minor*100000)) + $VERSIONCODE))
VERSIONCODE=$(($(($patch*1000)) + $VERSIONCODE))
VERSIONCODE=$(($(($dev)) + $VERSIONCODE))

# The 32 bit and 64 bit APKs each need there own version code unique version code
if [ "$1" = "32" ]; then
    VERSIONCODE=34$VERSIONCODE
else
    VERSIONCODE=66$VERSIONCODE
fi

MANIFEST_FILE=android/AndroidManifest.xml

# manifest package
if [ "$2" = "master" ]; then
	echo "Adjusting package name for daily build"
	QGC_PKG_NAME="org.mavlink.qgroundcontrolbeta"
	sed -i -e 's/package *= *"[^"]*"/package="'$QGC_PKG_NAME'"/' $MANIFEST_FILE
fi

# android:versionCode
if [ -n "$VERSIONCODE" ]; then
	echo "Android versionCode: ${VERSIONCODE}"
	sed -i -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"$VERSIONCODE\"/" $MANIFEST_FILE
else
	echo "Error: versionCode empty"
	exit 1
fi

# android:versionName
if [ -n "$VERSIONNAME" ]; then
	echo "Android versionName: ${VERSIONNAME}"
	sed -i -e 's/versionName *= *"[^"]*"/versionName="'$VERSIONNAME'"/' $MANIFEST_FILE
else
	echo "Error: versionName empty"
	exit 1
fi

