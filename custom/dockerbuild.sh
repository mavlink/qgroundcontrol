#!/bin/bash
case $0 in /*) SCRIPT="$0";; *) SCRIPT="`pwd`/${0#./}";; esac
ROOTDIR=`dirname "$SCRIPT"`
GSTSYMLINK=$ROOTDIR/../gstreamer-1.0-android-x86-1.5.2
[ -L $GSTSYMLINK ] && mv $GSTSYMLINK $GSTSYMLINK-orig
docker_repo=879326759580.dkr.ecr.eu-central-1.amazonaws.com/datapilot:latest
$(aws ecr get-login --region eu-central-1 --no-include-email)
docker pull $docker_repo
docker run -i -v `dirname $ROOTDIR`:/home/docker1000/qgroundcontrol:rw -v /tmp/datapilot_build:/tmp/datapilot_build:rw $docker_repo
[ -L $GSTSYMLINK ] && rm $GSTSYMLINK
[ -L $GSTSYMLINK-orig ] && mv $GSTSYMLINK-orig $GSTSYMLINK
