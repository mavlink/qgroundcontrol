#!/bin/bash
set -e
if [ ! $# -eq 2 ]; then
    echo 'usage: StripAndTarQtS3Min.sh qt_version target'
    echo 'example: StripAndTarQtS3Min.sh 5.12.10 gcc_64'
    exit 1
fi
QT_VERSION=$1
QT_TARGET=$2
QT_DIR=~/Qt
QT_FULL_VERSION_DIR=${QT_DIR}/${QT_VERSION}
STAGING_DIRNAME=Qt${QT_VERSION}-${QT_TARGET}-min
ZIP_FILENAME=Qt${QT_VERSION}-${QT_TARGET}-min.7z
if [ ! -d ${QT_FULL_VERSION_DIR} ] ; then
    echo 'Qt version directory not found'
    exit 1
fi
if [ -d ~/${STAGING_DIRNAME} ] ; then
    rm -rf ~/${STAGING_DIRNAME}
fi
if [ -f ~/${ZIP_FILENAME} ] ; then
    rm ~/${ZIP_FILENAME}
fi
mkdir -p ~/${STAGING_DIRNAME}
cp -r ${QT_FULL_VERSION_DIR}/${QT_TARGET}  ~/${STAGING_DIRNAME}
rm -rf ~/${STAGING_DIR}/${QT_FULL_VERSION}/${QT_TARGET}/doc
find ~/${STAGING_DIRNAME} -type f -name 'lib*.debug' -delete
pushd ${HOME}
7z a ${ZIP_FILENAME} ${STAGING_DIRNAME}
popd
