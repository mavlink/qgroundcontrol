#! /bin/bash

VERSIONCODE=`git rev-list master --first-parent --count`
VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`

echo "VersionCode: ${VERSIONCODE}"
echo "VersionName: ${VERSIONNAME}"

sed -i -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"${VERSIONCODE}\"/" android/AndroidManifest.xml
set -i -e "s/android:versionName=\".*\"/android:versionName=\"${VERSIONNAME}\"/" android/AndroidManifest.xml

