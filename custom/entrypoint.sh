#!/bin/bash

# Fail on error
set -e

# Add symlink for gstreamer
ln -f -s ~/dpbuild/gstreamer-1.0-android-x86-1.5.2

path_to_custom=/home/docker1000/qgroundcontrol/custom

# Set package name and version in AndroidManifest.xml
cp $path_to_custom/android_typhoonh/AndroidManifest.xml /tmp/AndroidManifest_DataPilot.copy
$path_to_custom/update_manifest.sh

function cleanup {
    echo "Restore AndroidManifest.xml"
    cp /tmp/AndroidManifest_DataPilot.copy $path_to_custom/android_typhoonh/AndroidManifest.xml
    rm -f /tmp/AndroidManifest_DataPilot.copy
}

trap cleanup INT TERM 

# Execute build commands
if [ "$1" = "developer" ]; then
    $path_to_custom/build.sh rebuild
elif [ "$1" = "playstore" ]; then

    if [ `ls $path_to_custom/ | grep datapilot_keystore.jks | wc -l` -ne 1 ]; then
        echo "Error: datapilot_keystore.jks not found in custom directory, exit"
        cleanup
        exit 1
    elif [ -z "$2" ]; then
        echo "Error: no signing key password provided, exit"
        cleanup
        exit 1
    elif [ -z "$3" ]; then
        echo "Error: no signing keystore password provided, exit."
        cleanup
        exit 1
    fi

    export KEYSTORE_FILE=$path_to_custom/datapilot_keystore.jks
    export KEYSTORE_USER=YuneecDataPilotKeystore
    export KEY_PWD="$2"
    export KEYSTORE_PWD="$3"

    $path_to_custom/build_playstore.sh rebuild
elif [ ! -z "$1" ]; then
    exec "$@"
else
    echo "Error: no valid argument for entrypoint script provided, exit."
    cleanup
    exit 1
fi

cleanup
