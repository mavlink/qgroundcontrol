#! /usr/bin/env bash

set -e

multipass launch qgc
multipass exec qgc -- git clone https://github.com/mavlink/qgroundcontrol.git --recurse-submodules
multipass exec qgc -- cd qgroundcontrol
multipass exec qgc -- sudo ./tools/setup/install-dependencies-debian.sh
multipass exec qgc -- sudo ./tools/setup/install-qt-debian.sh
multipass exec qgc -- cmake -S /project/source -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_STABLE_BUILD=OFF
multipass exec qgc -- cmake --build . --target all --config Debug
multipass exec qgc -- cmake --install . --config Debug
