#!/bin/bash
#----------------------------------------------------------
# You will need:
# - Qt 5.15.2 android kit
# - Android SDK
# - Androig NDK
# - jdk 11
#----------------------------------------------------------
# Update with correct location for these
export ANDROID_HOME=~/Library/Android/sdk
export ANDROID_SDK_ROOT=~/Library/Android/sdk
export ANDROID_NDK_ROOT=~/Library/Android/sdk/ndk-bundle
#----------------------------------------------------------
# To build it, run (replacing the path with where you have Qt installed)
#
# >source android_environment.sh
# cd ../
# mkdir android_build
# cd android_build
# >~/local/Qt/5.15.2/android/bin/qmake -r -spec android-clang CONFIG+=debug ANDROID_ABIS=armeabi-v7a ../qgroundcontrol/qgroundcontrol.pro
# >make -j24 apk
#
