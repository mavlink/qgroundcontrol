#!/bin/bash -x

set +e

# Generate AppImage using the binaries currently provided by the project.
# These require at least GLIBC 2.14, which older distributions might not have. 
# On the other hand, 2.14 is not that recent so maybe we can just live with it.

APP=qgroundcontrol

mkdir -p /tmp/$APP/$APP.AppDir
cd /tmp/$APP/
tar xf ${SHADOW_BUILD_DIR}/release/package/qgroundcontrol.tar.bz2

wget -c http://ftp.us.debian.org/debian/pool/main/u/udev/udev_175-7.2_amd64.deb
wget -c http://ftp.us.debian.org/debian/pool/main/e/espeak/espeak_1.46.02-2_amd64.deb
wget -c http://ftp.us.debian.org/debian/pool/main/libs/libsdl1.2/libsdl1.2debian_1.2.15-5_amd64.deb

cd $APP.AppDir/

mv ../qgroundcontrol/* .
mv qgroundcontrol-start.sh AppRun
find ../ -name *.deb -exec dpkg -x {} . \;

# Get icon
cp ${TRAVIS_BUILD_DIR}/resources/icons/qgroundcontrol.png .

cat > ./qgroundcontrol.desktop <<\EOF
[Desktop Entry]
Type=Application
Name=QGroundControl
GenericName=Ground Control Station
Comment=UAS ground control station
Icon=qgroundcontrol.png
Exec=AppRun
Terminal=false
Categories=Utility;
Keywords=computer;
EOF

VERSION=$(strings qgroundcontrol | grep v[0-9*]\.[0-9*]\.[0-9*]-[0-9*] | head -n 1)

# Go out of AppImage
cd ..

wget -c "https://github.com/probonopd/AppImageKit/releases/download/5/AppImageAssistant" # (64-bit)
chmod a+x ./AppImageAssistant
mkdir -p ../out
rm ../out/$APP".AppImage" || true
./AppImageAssistant ./$APP.AppDir/ ../out/$APP".AppImage"

# s3 deploys everything in release/package
cp ../out/$APP".AppImage" ${SHADOW_BUILD_DIR}/release/package/$APP".AppImage"

