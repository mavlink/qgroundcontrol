#!/usr/bin/env bash

# this requires `master` in the git tree
#  travis-ci branch builds are unable to set the version properly

PLIST_FILE_SRC=$1
PLIST_FILE_DST=$2

BUILD_CODE=`git rev-list master --first-parent --count`
VERSION_CODE=`git describe --always --tags | sed -e 's/[^0-9.]*\([0-9.]*\).*/\1/'`

if [ -z "$BUILD_CODE" -o -z "$VERSION_CODE" ]; then
    echo "Error: Version and/or build empty."
    exit 1 # Cause the build to fail
else
    echo "Version: ${VERSION_CODE}"
    echo "Build:   ${BUILD_CODE}"
fi

sed -e "s/\###BUILD###/${BUILD_CODE}/" -e "s/\###VERSION###/${VERSION_CODE}/" $PLIST_FILE_SRC > $PLIST_FILE_DST
