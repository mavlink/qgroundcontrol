#!/bin/bash
case $0 in /*) SCRIPT="$0";; *) SCRIPT="`pwd`/${0#./}";; esac
ROOTDIR=`dirname "$SCRIPT"`
GSTSYMLINK=$ROOTDIR/../gstreamer-1.0-android-x86-1.5.2
[ -L $GSTSYMLINK ] && mv $GSTSYMLINK $GSTSYMLINK-orig
docker run -it -v `dirname $ROOTDIR`:/home/docker1000/qgroundcontrol:rw datapilot
[ -L $GSTSYMLINK ] && rm $GSTSYMLINK
[ -L $GSTSYMLINK-orig ] && mv $GSTSYMLINK-orig $GSTSYMLINK
