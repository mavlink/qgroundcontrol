#!/bin/bash

# How to run
# docker run -v ${PWD}:/scripts --rm -it mavlink/qgc-build-android /scripts/build_gdal.sh
# This will buid the libs in the current folder

apt update
apt install -y python pkg-config automake autoconf autogen libtool

/opt/android-ndk/build/tools/make_standalone_toolchain.py --arch=arm64 --api=21 --install-dir=/android-21-toolchain

export ANDROID_NDK='/opt/android-ndk'
export ANDROID_SDK='/opt/android-sdk'
export PATH=$PATH:/android-21-toolchain/bin/


## Compile PROJ

git clone https://github.com/OSGeo/PROJ/
cd PROJ/
git checkout 5.2.0
./autogen.sh
CC="aarch64-linux-android-clang" CXX="aarch64-linux-android-clang++" CFLAGS=" -D__ANDROID_API__=21 " CXXFLAGS=" -D__ANDROID_API__=21" ./configure --host=aarch64-linux-android \
 --prefix=$PROJECT/external/proj 
make -j16
make install
mkdir /scripts/arm64
cp -r /external/proj/* /scripts/arm64

cd /scripts

## Compile OpenSSL
#git clone https://github.com/openssl/openssl/
#cd openssl
#git checkout OpenSSL_1_0_2
#export CROSS_COMPILE=aarch64-linux-android-
##export ARCH=arm64
##./scripts/Setenv-android.sh
##/Configure linux-aarch64 --cross-compile-prefix=${CROSS_COMPILE} --prefix=/external/openssl shared
##CC="aarch64-linux-android-clang" CXX="aarch64-linux-android-clang++" CFLAGS="-mthumb -D__ANDROID_API__=21 " CXXFLAGS="-mthumb -D__ANDROID_API__=21" ./configure --host=aarch64-linux-android \
##CFLAGS="-mthumb -D__ANDROID_API__=21 " CXXFLAGS="-mthumb -D__ANDROID_API__=21" LIBS+=" -lsupc++ -lstdc++" ./Configure linux-aarch64 --cross-compile-prefix=${CROSS_COMPILE} --prefix=/external/openssl shared
##make depend
##make -j16
##make install
##make CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=" build_libs
#./Configure android shared
#make build_libs
#
#cp /scripts/arm64/lib/libssl.so /android-21-toolchain/sysroot/usr/lib/
#cp /scripts/arm64/lib/libcrypto.so /android-21-toolchain/sysroot/usr/lib/

wget https://github.com/christian-mv/android-openssl-binaries/raw/master/openssl-1.0.2r/arm64/libcrypto.so
cp libcrypto.so /scripts/arm64/lib
wget https://github.com/christian-mv/android-openssl-binaries/raw/master/openssl-1.0.2r/arm64/libssl.so
cp libssl.so /scripts/arm64/lib

## Install everything ( Proj needs to be in /scripts/arm64 as well )
cp -r /scripts/arm64/lib/* /android-21-toolchain/sysroot/usr/lib
cp -r /scripts/arm64/include/* /android-21-toolchain/sysroot/usr/include


## Compile GDAL

git clone https://github.com/OSGeo/gdal.git
cd gdal/gdal
git checkout v2.4.2
git apply /scripts/gdal.patch

./autogen.sh

CC="aarch64-linux-android-clang" CXX="aarch64-linux-android-clang++" CFLAGS=" -D__ANDROID_API__=21 " CXXFLAGS=" -D__ANDROID_API__=21" LIBS=" -lsupc++ -lstdc++ -L/scripts/arm64/lib/ -lcrypto -lssl " ./configure --host=aarch64-linux-android \
 --prefix=$PROJECT/external/gdal \
 --with-libtiff=internal \
 --with-geotiff=internal \
 --with-libjson-c=internal \
 --with-static-proj4=/external/proj/ \
 --with-hide-internal-symbols=yes \
 --with-threads  \
 --with-libz=internal  \
 --with-geos \
 --without-sse  \
 --without-ssse3  \
 --without-avx \
 --without-liblzma  \
 --without-pg  \
 --without-grass  \
 --without-libgrass  \
 --without-cfitsio  \
 --without-pcraster  \
 --without-png \
 --without-dds \
 --without-gta \
 --without-pcidsk \
 --without-jpeg      \
 --without-gif  \
 --without-ogdi      \
 --without-fme        \
 --without-sosi         \
 --without-mongocxx        \
 --without-boost-lib-path   \
 --without-hdf4      \
 --without-hdf5      \
 --without-kea       \
 --without-netcdf    \
 --without-jasper    \
 --without-openjpeg  \
 --without-fgdb      \
 --without-ecw       \
 --without-kakadu    \
 --without-mrsid       \
 --without-jp2mrsid    \
 --without-mrsid_lidar  \
 --without-msg           \
 --without-bsb        \
 --without-oci        \
 --without-grib           \
 --without-gnm             \
 --without-mysql  \
 --without-ingres \
 --without-xerces \
 --without-expat  \
 --without-libkml  \
 --without-odbc \
 --without-curl \
 --without-xml2 \
 --without-spatialite \
 --without-sqlite3 \
 --without-rasterlite2 \
 --without-pcre \
 --without-teigha  \
 --without-idb \
 --without-sde \
 --without-epsilon \
 --without-webp \
 --without-sfcgal \
 --without-qhull \
 --without-opencl \
 --without-freexl \
 --without-pam      \
 --without-poppler  \
 --without-podofo \
 --without-pdfium \
 --without-macosx-framework    \
 --without-perl            \
 --without-python \
 --without-java     \
 --without-mdb       \
 --without-jvm-lib \
 --without-rasdaman   \
 --without-armadillo  \
 --without-cryptopp   \
 --without-mrf

make -j8
make install
cp -r /external/gdal/* /scripts/arm64
