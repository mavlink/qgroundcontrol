#!/bin/sh

# Fail on error
set -e

# Add symlink for gstreamer
ln -f -s ~/dpbuild/gstreamer-1.0-android-x86-1.5.2

# Execute build commands
if [ "$1" = "developer" ]; then
    /home/docker1000/qgroundcontrol/custom/build.sh rebuild
elif [ "$1" = "playstore" ]; then

    if [ `ls /home/docker1000/qgroundcontrol/custom/ | grep datapilot_keystore.jks | wc -l` -ne 1 ]; then
        echo "Error: datapilot_keystore.jks not found in custom directory, exit"
        exit 1
    elif [ -z "$2" ]; then
        echo "Error: no signing key password provided, exit"
        exit 1
    elif [ -z "$3" ]; then
        echo "Error: no signing keystore password provided, exit."
        exit 1
    fi

    export KEYSTORE_FILE=/home/docker1000/qgroundcontrol/custom/datapilot_keystore.jks
    export KEYSTORE_USER=YuneecDataPilotKeystore
    export KEY_PWD="$2"
    export KEYSTORE_PWD="$3"

    /home/docker1000/qgroundcontrol/custom/build_playstore.sh rebuild
elif [ ! -z "$1" ]; then
    exec "$@"
else
    echo "Error: no valid argument for entrypoint script provided, exit."
    exit 1
fi
