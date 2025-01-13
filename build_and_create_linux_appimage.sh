#!/bin/bash

set -e

docker build --file ./deploy/docker/Dockerfile-build-linux -t qgc-ubuntu-docker .

if [ -d build ]; then
	rm -r build
fi
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
./deploy/create_linux_appimage.sh . build/staging
