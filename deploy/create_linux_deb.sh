#! /bin/bash

if [[ $# -eq 0 ]]; then
  echo 'create_linux_deb.sh QGCBINARY CONTROLFILE OUTPUT'
  exit 1
fi
tmpdir=`mktemp -d`


cd ${tmpdir}
mkdir QGroundControl
cd QGroundControl
mkdir usr
mkdir usr/local
mkdir usr/local/bin

cp $1 ./usr/local/bin

mkdir DEBIAN
cp $2 ./DEBIAN/control

cd ${tmpdir}

dpkg-deb --build QGroundControl
cp QGroundControl.deb $3
