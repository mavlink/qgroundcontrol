#! /bin/bash

# absolute directory of this script
CURDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

MAVLINK_DIR=$CURDIR/../libs/mavlink/include/mavlink

# tmpdir creation on both linux and osx
TMPDIR=`mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir'`

cd $TMPDIR
git clone --recursive https://github.com/mavlink/mavlink.git
cd mavlink

# mavlink v1.0
rm -rf $MAVLINK_DIR/v1.0/*
python -m pymavlink.tools.mavgen -o $MAVLINK_DIR/v1.0/ --lang C --wire-protocol 1.0 message_definitions/v1.0/ardupilotmega.xml
python -m pymavlink.tools.mavgen -o $MAVLINK_DIR/v1.0/ --lang C --wire-protocol 1.0 message_definitions/v1.0/common.xml

# mavlink v2.0
rm -rf $MAVLINK_DIR/v2.0/*
python -m pymavlink.tools.mavgen -o $MAVLINK_DIR/v2.0/ --lang C --wire-protocol 2.0 message_definitions/v1.0/ardupilotmega.xml
python -m pymavlink.tools.mavgen -o $MAVLINK_DIR/v2.0/ --lang C --wire-protocol 2.0 message_definitions/v1.0/common.xml

