#!/bin/sh

# Something is wrong !
error() {
    echo -e "ERROR: $*" >&2
    exit 1
}

echob() {
    echo $(tput bold)"$@"$(tput sgr0)
}

# Get script name
PROG=$(basename $0)
usage() {
    echob "USAGE: $PROG BUILD_FOLDER"
}

# Set vars
BASEDIR=$(dirname $(readlink -f "$0"))/..
BUILDDIR=$(readlink -f "$1")
DEPLOYDIR="$BUILDDIR/deploy"
LOCALDIR=$PWD

PLUGINS="imageformats/libqsvg.so,texttospeech/libqttexttospeech_flite.so,texttospeech/libqtexttospeech_speechd.so"

# Check args
if [ $# -eq 1 ]; then
    TARGET="$1"
else
    usage && error "Incorrect number of arguments"
fi

# Check if argument path exist
if [ ! -d $BUILDDIR ]; then
    usage && error "$BUILDDIR does not exist."
fi

# Remove last deploy
if [ -d "$BUILDDIR/deploy" ]; then
    echob "Remove last deploy folder.."
    rm -rf "$BUILDDIR/deploy"
fi

echob "Create deploy dir.."
mkdir "$DEPLOYDIR"

echob "Copy some applications files.."
cp "${BASEDIR}/deploy/qgroundcontrol.desktop" "$DEPLOYDIR"
cp "${BASEDIR}/resources/icons/qgroundcontrol.png" "$DEPLOYDIR"
cp "${BUILDDIR}/release/QGroundControl" "$DEPLOYDIR"

echob "Create lib dir.."
mkdir "$DEPLOYDIR/lib/"

echob "Copy gstreamer binaries.."
ls -1 -d /usr/lib/x86_64-linux-gnu/* | grep -x ".*libgst.*-1.0.so" | xargs cp -t "$DEPLOYDIR/lib/"

echob "Prepare linuxdeployqt.."
if [ ! -f /tmp/linuxdeployqt*.AppImage ]; then
    echob "Download.."
    wget "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -P /tmp/;
    chmod a+x /tmp/linuxdeployqt*.AppImage;
fi

echob "Create AppImage"
/tmp/linuxdeployqt*.AppImage "$DEPLOYDIR/QGroundControl" -verbose=2 "-qmldir=$BASEDIR/src" -bundle-non-qt-libs -extra-plugins=$PLUGINS;
/tmp/linuxdeployqt*.AppImage "$DEPLOYDIR/qgroundcontrol.desktop" -verbose=2 -bundle-non-qt-libs "-qmldir=$BASEDIR/src" -appimage;
echob "done !"
