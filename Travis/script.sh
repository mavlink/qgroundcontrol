 #!/bin/bash
set -e
cd ${TRAVIS_BUILD_DIR}
./tools/update_android_version.sh
echo 'Building QGroundControl'
echo -en 'travis_fold:start:script.1\\r'
if [ "${CONFIG}" = "doxygen" ]; then
    cd ${TRAVIS_BUILD_DIR}/src
    doxygen documentation.dox
elif
    cd ${SHADOW_BUILD_DIR}
    if [ "${SPEC}" = "ios" ]; then
        xcodebuild -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
    elif
        make -j4
    fi
fi
echo -en 'travis_fold:end:script.1\\r'
if [[ "${TRAVIS_OS_NAME}" = "linux" && "${CONFIG}" = "debug" ]]; then
    ${SHADOW_BUILD_DIR}/debug/qgroundcontrol --unittest
fi
if [[ "${TRAVIS_OS_NAME}" = "osx" && "${CONFIG}" = "debug" ]]; then
    ${SHADOW_BUILD_DIR}/debug/qgroundcontrol.app/Contents/MacOS/qgroundcontrol --unittest
fi
