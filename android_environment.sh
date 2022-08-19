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
export ANDROID_HOME=C:\Users\yoony\AppData\Local\Android\Sdk
export ANDROID_SDK_ROOT=C:\Users\yoony\AppData\Local\Android\Sdk
export ANDROID_NDK_ROOT=C:\android-ndk-r20b
export ANDROID_NDK_HOST=windows-x86_64
export ANDROID_NDK_PLATFORM=C:\android-ndk-r20b\platforms\android-29
export ANDROID_NDK_TOOLCHAIN_PREFIX=aarch64-linux-android
export ANDROID_NDK_TOOLCHAIN_VERSION=4.9
export ANDROID_NDK_TOOLS_PREFIX=aarch64-linux-android
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
