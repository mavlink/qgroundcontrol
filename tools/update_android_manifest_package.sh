#!/usr/bin/env bash

MANIFEST_FILE=android/AndroidManifest.xml

echo "Adjusting package name for daily build"
QGC_PKG_NAME="org.mavlink.qgroundcontrolbeta"
sed -i -e 's/package *= *"[^"]*"/package="'$QGC_PKG_NAME'"/' $MANIFEST_FILE
