#! /bin/bash

# absolute directory of this script
CURDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

GPSDRIVERS_DIR=$CURDIR/../src/GPS/Drivers

# tmpdir creation on both linux and osx
TMPDIR=`mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir'`

cd $TMPDIR
git clone --recursive https://github.com/PX4/GpsDrivers.git

rsync -av --delete --exclude=.git GpsDrivers/ $GPSDRIVERS_DIR/

