#!/bin/bash
#
# Author: Gus Grubba <mavlink@grubba.com>
#
# Copies the regular distribution of the gstreamer framework
# into the libs directory (to be used for creating the Mac OS
# bundle). It also prunes unnecessary files and relocates the
# dynamic libraries.
# 
# This script is run by the build process and should not be
# executed manually.
#
# Usage: $script [framework destination path] [app bundle path] [qgc executable name]
#
# destination is usually:       ../libs/lib/Frameworks/
# app bundle is something like: /path/qgroundcontrol.app
# executable name is usually:   qgroundcontrol

die () {
    echo "$@" 1>&2
    exit 1
}

GST_VER=1.0
GST_ROOT=/Library/Frameworks/GStreamer.framework
GST_BASE=$GST_ROOT/Versions/$GST_VER
RELOC=$(dirname $0)/osxrelocator.py

echo "GST Installer"
[ "$#" -eq 3 ] || die "3 arguments required, $# provided"
[ -d "$1" ] || die "Could not find $1"
[ -d "$2" ] || die "Could not find $2"

FMWORK_TARGET=$1
BUNDLE_TARGET=$2
GST_SOURCE=${1%/}/GStreamer.framework
GST_TARGET=$GST_SOURCE/Versions/$GST_VER
QGC_BINARY=$BUNDLE_TARGET/Contents/MacOS/$3
[ -e "$QGC_BINARY" ] || die "Could not find $QGC_BINARY"

process_framework() {
    #-- Start looking for the source framework
    [ -d "$GST_ROOT" ] || die "Could not find gstreamer framework in $GST_ROOT"
    [ -d "$GST_BASE" ] || die "Wrong version of gstreamer framework found in $GST_ROOT"
    [ -e $RELOC ] || die "Cannot find $RELOC"
    echo "GST Installer: Copying $GST_ROOT to $FMWORK_TARGET"
    rsync -a --delete "$GST_ROOT" "$FMWORK_TARGET" || die "Error copying $GST_ROOT to $FMWORK_TARGET"
    #-- Prune unused stuff
    rm -rf $GST_TARGET/bin/*
    rm -rf $GST_TARGET/Headers/*
    rm -rf $GST_TARGET/include/*
    rm -rf $GST_TARGET/lib/*.a
    rm -rf $GST_TARGET/lib/*.la
    rm -rf $GST_TARGET/lib/gio/modules/static
    rm -rf $GST_TARGET/lib/glib-2.0
    rm -rf $GST_TARGET/lib/gst-validate-launcher
    rm -rf $GST_TARGET/lib/gstreamer-1.0/static
    rm -rf $GST_TARGET/lib/libffi-3.0.13
    rm -rf $GST_TARGET/lib/pkgconfig
    #-- Some dylibs are dupes instead of symlinks.
    #-- This will do a minimum job in trying to clean those.
    #-- Doesn't work. The stupid thing can't load a dlyb symlink.
    #for f in $GST_TARGET/lib/*.dylib
    #do
    #    foo=$(basename "$f")
    #    bar="${foo%.*}"
    #    for i in `seq 0 9`;
    #    do
    #        if [ -e $GST_TARGET/lib/$bar.$i.dylib ]; then
    #            DUPES="$DUPES
    #rm -f $GST_TARGET/lib/$bar.$i.dylib"
    #            DUPES="$DUPES
    #ln -s $f $GST_TARGET/lib/$bar.$i.dylib"
    #        fi
    #    done
    #done
    #IFS=$'\n'
    #for c in $DUPES
    #do
    #    eval $c
    #done
    #-- Now relocate the embeded paths
    echo "GST Installer: Relocating"
    python $RELOC -r $GST_TARGET/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ > /dev/null || die "Error relocating binaries in $GST_TARGET/lib"
    python $RELOC -r $GST_TARGET/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ > /dev/null || die "Error relocating binaries in $GST_TARGET/libexec"
}

#-- Check and see if we've already processed the framework
echo "GST Installer: Checking $GST_TARGET"
[ -d $GST_TARGET ] || process_framework
#-- Now copy the framework to the app bundle
echo "GST Installer: Copying $GST_SOURCE to $BUNDLE_TARGET/Contents/Frameworks/"
rsync -a --delete $GST_SOURCE $BUNDLE_TARGET/Contents/Frameworks/ || die "Error copying framework into app bundle"
#-- Move this gst binary to MacOS
mv $BUNDLE_TARGET/Contents/Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-plugin-scanner $BUNDLE_TARGET/Contents/MacOS/ || die "Error moving gst-plugin-scanner"
#-- Fix main binary
python $RELOC $QGC_BINARY /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ > /dev/null || die "Error relocating $QGC_BINARY"


