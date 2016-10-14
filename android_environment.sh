#!/bin/bash
#----------------------------------------------------------
# You will need:
# - Qt 5.5.x android_armv7 kit
# - Android SDK
# - Androig NDK
# - Current Java
# - ant
#----------------------------------------------------------
# Update with correct location for these
export ANDROID_HOME=~/Library/Android/sdk
export ANDROID_SDK_ROOT=~/Library/Android/sdk
export ANDROID_NDK_ROOT=~/Library/Android/sdk/ndk-bundle
export ANDROID_NDK_HOST=darwin-x86_64
export ANDROID_NDK_PLATFORM=/android-9
export ANDROID_NDK_TOOLCHAIN_PREFIX=arm-linux-androideabi
export ANDROID_NDK_TOOLCHAIN_VERSION=4.9
export ANDROID_NDK_TOOLS_PREFIX=arm-linux-androideabi
#----------------------------------------------------------
# To build it, run (replacing the path with where you have Qt installed)
#
# >source android_environment.sh
# cd ../
# mkdir android_build
# cd android_build
# >~/local/Qt/5.4/android_armv7/bin/qmake -r -spec android-g++ CONFIG+=debug ../qgroundcontrol/qgroundcontrol.pro
# >make -j24 install INSTALL_ROOT=./android-build/
# >~/local/Qt/5.4/android_armv7/bin/androiddeployqt --input ./android-libQGroundControl.so-deployment-settings.json --output ./android-build --deployment bundled --android-platform android-22 --jdk /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Home --verbose --ant /usr/local/bin/ant
#
