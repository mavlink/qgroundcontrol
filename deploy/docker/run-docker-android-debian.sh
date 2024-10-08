#!/usr/bin/env bash

# Run this from root directory

set -e

docker build --file ./deploy/docker/Dockerfile-build-android-debian -t qgc-android-debian-docker . --no-cache
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-android-debian-docker
