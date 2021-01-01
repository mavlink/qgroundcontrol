#!/bin/bash
set -e
if [ ! $# -eq 3 ]; then
    echo 'usage: StripAndTarQtS3Min.sh qt_dir qt_version target'
    echo 'example: StripAndTarQtS3Min.sh ~/Qt 5.12.10 gcc_64'
    exit 1
fi
QT_DIR=$1
QT_VERSION=$2
QT_TARGET=$3
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
find ~/${STAGING_DIRNAME} -path '*/*.framework.dSYM/*' -delete
find ~/${STAGING_DIRNAME} -type d -name '*.framework.dSYM' -delete
find ~/${STAGING_DIRNAME} -type f -name 'lib*_debug.*' -delete
pushd ${HOME}
7z a ${ZIP_FILENAME} ${STAGING_DIRNAME}
popd
