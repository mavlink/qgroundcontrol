set -e
if [[ "${TRAVIS_OS_NAME}" = "linux" && "${CONFIG}" != "doxygen" ]]; then
    wget https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/Qt5.5.1-linux.tar.bz2
    tar jxf Qt5.5.1-linux.tar.bz2 -C /tmp
    export PATH=/tmp/Qt/5.5/gcc_64/bin:$PATH
    export CXX="g++-4.8"
    export CC="gcc-4.8"
    export DISPLAY=:99.0
    sh -e /etc/init.d/xvfb start
elif [ "${TRAVIS_OS_NAME}" = "osx" ]; then
    if [ "${SPEC}" = "ios" ]; then
        # Install handcrafted Qt ios version
        wget https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/Qt5.5.1-ios.tar.bz2
        mkdir -p ~/qt-ios
        tar jxf Qt5.5.1-ios.tar.bz2 -C ~/qt-ios
        export PATH=~/qt-ios/ios/bin:$PATH
    elif
        # Install Qt clang version
        brew update
        brew install qt5
        export PATH=/usr/local/opt/qt5/bin:$PATH
    fi
elif [ "${TRAVIS_OS_NAME}" = "android" ]; then
    wget http://dl.google.com/android/ndk/android-ndk-r10e-linux-x86_64.bin
    chmod +x android-ndk-r10e-linux-x86_64.bin
    ./android-ndk-r10e-linux-x86_64.bin > /dev/null
    export PATH=`pwd`/android-ndk-r10e:$PATH
    export ANDROID_NDK_ROOT=`pwd`/android-ndk-r10e
    export ANDROID_SDK_ROOT=/usr/local/android-sdk
    wget https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/Qt5.5.1-linux.tar.bz2
    tar jxf Qt5.5.1-linux.tar.bz2 -C /tmp
    export PATH=/tmp/Qt/5.5/android_armv7/bin:$PATH
fi

