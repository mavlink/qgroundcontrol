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
find . ! -name 'clang_64' -maxdepth 1 -type d -exec rm -rf {} +
cd clang_64
rm -rf doc
cd lib
rm -rf *.dSYM
cd $1
cd ..
tar -jcvf Qt5.8.0-mac-clang-min.tar.bz2 $1
