#! /bin/bash

MANIFEST_FILE=android/AndroidManifest.xml

VERSIONCODE=`git rev-list master --first-parent --count`
VERSIONNAME=`git describe --always --tags | sed -e 's/^v//'`

echo "VersionCode: ${VERSIONCODE}"
echo "VersionName: ${VERSIONNAME}"

sed -i -e "s/android:versionCode=\"[0-9][0-9]*\"/android:versionCode=\"${VERSIONCODE}\"/" $MANIFEST_FILE
sed -i -e 's/versionName *= *"[^"]*"/versionName="'$VERSIONNAME'"/' $MANIFEST_FILE
