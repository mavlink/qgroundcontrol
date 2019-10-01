#!/bin/bash

# How to run
# docker run -v/PATH-TO-SCRIPTS-FOLDER/scripts/:/scripts --rm -it mavlink/qgc_android /scripts/build_gdal.sh
# This will create a ".libs" folder in scripts directory containing libgdal

apt update
apt install -y python pkg-config automake

/opt/android-ndk/build/tools/make-standalone-toolchain.sh --platform=android-21 --install-dir=/android-21-toolchain

export ANDROID_NDK='/opt/android-ndk'
export ANDROID_SDK='/opt/android-sdk'
export PATH=$PATH:/android-21-toolchain/bin/

git clone https://github.com/OSGeo/gdal.git
cd gdal/gdal
git checkout v2.4.2
git apply /scripts/gdal.patch

./autogen.sh

CC="arm-linux-androideabi-clang" CXX="arm-linux-androideabi-clang++" CFLAGS="-mthumb -D__ANDROID_API__=21 " CXXFLAGS="-mthumb -D__ANDROID_API__=21" LIBS=" -lsupc++ -lstdc++"       ./configure --host=arm-linux-androideabi --without-grib --prefix=$PROJECT/external/gdal 

make -j8
#make install
cp -r .libs /scripts
