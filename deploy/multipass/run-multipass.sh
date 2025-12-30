#! /usr/bin/env bash

set -e

multipass launch qgc
multipass exec qgc -- git clone https://github.com/mavlink/qgroundcontrol.git --recurse-submodules
multipass exec qgc -- sudo qgroundcontrol/tools/setup/install-dependencies-debian.sh
multipass exec qgc -- sudo qgroundcontrol/tools/setup/install-qt-debian.sh
multipass exec qgc -- mkdir qgroundcontrol/build
multipass exec qgc -- cmake -S qgroundcontrol -B qgroundcontrol/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=OFF -DQGC_STABLE_BUILD=OFF
multipass exec qgc -- cmake --build qgroundcontrol/build --target all --config Release
multipass exec qgc -- cmake --install qgroundcontrol/build --config Release
multipass transfer qgc:qgroundcontrol/build/QGroundControl-x86_64.AppImage .
