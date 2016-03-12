#!/bin/bash
set -e
# setup ccache
mkdir -p ~/bin
ln -s /usr/bin/ccache ~/bin/g++
ln -s /usr/bin/ccache ~/bin/g++-4.8
ln -s /usr/bin/ccache ~/bin/gcc
ln -s /usr/bin/ccache ~/bin/gcc-4.8
export PATH=~/bin:$PATH
if [[ "${TRAVIS_OS_NAME}" = "android" && "${CONFIG}" = "installer" && -z ${ANDROID_STOREPASS} ]]; then
    export CONFIG=release
fi
if [ "${CONFIG}" != "doxygen" ]; then
    mkdir ${SHADOW_BUILD_DIR}
    cd ${SHADOW_BUILD_DIR}
    if [ "${SPEC}" = "ios" ]; then
        qmake -r ${TRAVIS_BUILD_DIR}/qgroundcontrol.pro CONFIG+=WarningsAsErrorsOn CONFIG-=debug_and_release CONFIG+=release
    elif
        qmake -r ${TRAVIS_BUILD_DIR}/qgroundcontrol.pro CONFIG+=${CONFIG} CONFIG+=WarningsAsErrorsOn -spec ${SPEC}
    fi
fi
