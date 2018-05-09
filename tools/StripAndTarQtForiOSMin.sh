#!/bin/bash
if [ $# -eq 0 ]; then
    echo 'Qt directory must be specified as argument'
    exit 1
fi
if [ ! -d $1 ] ; then
    echo 'Qt directory not found'
    exit 1
fi
cd $1
rm *
find . ! -name '5.8' -maxdepth 1 -type d -exec rm -rf {} +
cd 5.8
find . ! -name 'ios' -maxdepth 1 -type d -exec rm -rf {} +
cd ios
rm -rf doc
find . -type f -name 'lib*_debug.a' -delete
find . -type f -name 'lib*_debug.la' -delete
find . -type f -name 'lib*_debug.prl' -delete
cd ..
cd ..
cd ..
tar -jcvf Qt5.8.0-mac-ios-min.tar.bz2 $1
