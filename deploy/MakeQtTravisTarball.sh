#!/bin/bash -x

if [[ $# -eq 0 ]]; then
        echo 'MakeQtTravisTarball.sh QtDirectory QtFullVersion QtBaseVersion BuildType'
        exit 1
fi

QT_DIRECTORY=$1
if [ ! -d ${QT_DIRECTORY} ]; then
        echo 'Specify directory for Qt Directory.'
        exit 1
fi

QT_FULL_VERSION=$2
if [ ! -d ${QT_DIRECTORY}/${QT_FULL_VERSION} ]; then
        echo 'Qt version directory not found'
        exit 1
fi

QT_BASE_VERSION=$3

QT_BUILD_TYPE=$4
if [ ! -d ${QT_DIRECTORY}/${QT_FULL_VERSION}/${QT_BUILD_TYPE} ]; then
        echo 'Qt build type directory not found'
        exit 1
fi

mkdir -p Qt${QT_BASE_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}/${QT_BUILD_TYPE}
cp -r ${QT_DIRECTORY}/${QT_FULL_VERSION}/${QT_BUILD_TYPE} Qt${QT_BASE_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}
rm -rf Qt${QT_BASE_VERSION}-${QT_BUILD_TYPE}/${QT_FULL_VERSION}/${QT_BUILD_TYPE}/doc
tar -jcvf Qt${QT_FULL_VERSION}-${QT_BUILD_TYPE}-min.tar.bz2 Qt${QT_BASE_VERSION}-${QT_BUILD_TYPE}/
