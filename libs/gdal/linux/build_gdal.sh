#!/bin/bash

# How to run
# docker run -v ${PWD}:/scripts --rm -it mavlink/qgc-build-linux /scripts/build_gdal.sh
# This will buid the libs in the current folder

apt update
apt install -y python pkg-config automake autoconf autogen libtool

## Compile PROJ

git clone https://github.com/OSGeo/PROJ/
cd PROJ/
git checkout 5.2.0
./autogen.sh
./configure \
 --prefix=$PROJECT/external/proj 
make -j16
make install
cp -r /external/proj/* /scripts

cd /scripts

## Compile GDAL

git clone https://github.com/OSGeo/gdal.git
cd gdal/gdal
git checkout v2.4.2
git apply /scripts/gdal.patch

./autogen.sh

./configure \
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

make -j16
make install
cp -r /external/gdal/* /scripts
