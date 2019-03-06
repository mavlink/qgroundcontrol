#!/bin/bash -x

#set +e

if [[ $# -eq 0 ]]; then
  echo 'create_linux_appimage.sh QGC_SRC_DIR QGC_RELEASE_DIR'
  exit 1
fi

QGC_SRC=$1

QGC_CUSTOM_APP_NAME="${QGC_CUSTOM_APP_NAME:-QGroundControl}"
QGC_CUSTOM_GENERIC_NAME="${QGC_CUSTOM_GENERIC_NAME:-Ground Control Station}"
QGC_CUSTOM_BINARY_NAME="${QGC_CUSTOM_BINARY_NAME:-QGroundControl}"
QGC_CUSTOM_LINUX_START_SH="${QGC_CUSTOM_LINUX_START_SH:-${QGC_SRC}/deploy/qgroundcontrol-start.sh}"
QGC_CUSTOM_APP_ICON="${QGC_CUSTOM_APP_ICON:-${QGC_SRC}/resources/icons/qgroundcontrol.png}"
QGC_CUSTOM_APP_ICON_NAME="${QGC_CUSTOM_APP_ICON_NAME:-QGroundControl}"

if [ ! -f ${QGC_SRC}/qgroundcontrol.pro ]; then
  echo "please specify path to $(QGC_CUSTOM_APP_NAME) source as the 1st argument"
  exit 1
fi

QGC_RELEASE_DIR=$2
if [ ! -f ${QGC_RELEASE_DIR}/${QGC_CUSTOM_BINARY_NAME} ]; then
  echo "please specify path to ${QGC_CUSTOM_BINARY_NAME} release as the 2nd argument"
  exit 1
fi

OUTPUT_DIR=${3-`pwd`}
echo "Output directory:" ${OUTPUT_DIR}

# Generate AppImage using the binaries currently provided by the project.
# These require at least GLIBC 2.14, which older distributions might not have.
# On the other hand, 2.14 is not that recent so maybe we can just live with it.

APP=${QGC_CUSTOM_BINARY_NAME}

TMPDIR=`mktemp -d`
APPDIR=${TMPDIR}/$APP".AppDir"
mkdir -p ${APPDIR}

cd ${TMPDIR}
wget -c --quiet http://ftp.us.debian.org/debian/pool/main/u/udev/udev_175-7.2_amd64.deb
wget -c --quiet http://ftp.us.debian.org/debian/pool/main/s/speech-dispatcher/speech-dispatcher_0.8.8-1_amd64.deb
wget -c --quiet http://ftp.us.debian.org/debian/pool/main/libs/libsdl2/libsdl2-2.0-0_2.0.2%2bdfsg1-6_amd64.deb

cd ${APPDIR}
find ../ -name *.deb -exec dpkg -x {} . \;

# copy libdirectfb-1.2.so.9
cd ${TMPDIR}
wget -c --quiet http://ftp.us.debian.org/debian/pool/main/d/directfb/libdirectfb-1.2-9_1.2.10.0-5.1_amd64.deb
mkdir libdirectfb
dpkg -x libdirectfb-1.2-9_1.2.10.0-5.1_amd64.deb libdirectfb
cp -L libdirectfb/usr/lib/x86_64-linux-gnu/libdirectfb-1.2.so.9 ${APPDIR}/usr/lib/x86_64-linux-gnu/
cp -L libdirectfb/usr/lib/x86_64-linux-gnu/libfusion-1.2.so.9 ${APPDIR}/usr/lib/x86_64-linux-gnu/
cp -L libdirectfb/usr/lib/x86_64-linux-gnu/libdirect-1.2.so.9 ${APPDIR}/usr/lib/x86_64-linux-gnu/

# copy libts-0.0-0
wget -c --quiet http://ftp.us.debian.org/debian/pool/main/t/tslib/libts-0.0-0_1.0-11_amd64.deb
mkdir libts
dpkg -x libts-0.0-0_1.0-11_amd64.deb libts
cp -L libts/usr/lib/x86_64-linux-gnu/libts-0.0.so.0 ${APPDIR}/usr/lib/x86_64-linux-gnu/

# copy QGroundControl release into appimage
rsync -av --exclude=*.cpp --exclude=*.h --exclude=*.o --exclude="CMake*" --exclude="*.cmake" ${QGC_RELEASE_DIR}/* ${APPDIR}/
rm -rf ${APPDIR}/package
cp ${QGC_CUSTOM_LINUX_START_SH} ${APPDIR}/AppRun

# copy icon
cp ${QGC_CUSTOM_APP_ICON} ${APPDIR}/

cat > ./QGroundControl.desktop <<\EOF
[Desktop Entry]
Type=Application
Name=${QGC_CUSTOM_APP_NAME}
GenericName=${QGC_CUSTOM_GENERIC_NAME}
Comment=UAS ground control station
Icon=${QGC_CUSTOM_APP_ICON_NAME}
Exec=AppRun
Terminal=false
Categories=Utility;
Keywords=computer;
EOF

VERSION=$(strings ${APPDIR}/${QGC_CUSTOM_BINARY_NAME} | grep '^v[0-9*]\.[0-9*].[0-9*]' | head -n 1)
echo ${QGC_CUSTOM_APP_NAME} Version: ${VERSION}

# Go out of AppImage
cd ${TMPDIR}
wget -c --quiet "https://github.com/probonopd/AppImageKit/releases/download/5/AppImageAssistant" # (64-bit)
chmod a+x ./AppImageAssistant

./AppImageAssistant ./$APP.AppDir/ ${TMPDIR}/$APP".AppImage"

cp ${TMPDIR}/$APP".AppImage" ${OUTPUT_DIR}/$APP".AppImage"

