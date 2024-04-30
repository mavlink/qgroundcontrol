#! /usr/bin/env bash

#Run this from root directory

set -e

docker build --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
docker run --cap-add SYS_ADMIN --device /dev/fuse --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
