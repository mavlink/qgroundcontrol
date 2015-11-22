# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2015 QGroundControl Developers
#
# This file is part of the open groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
#
# Author: Gus Grubba <mavlink@grubba.com>
# -------------------------------------------------

#
#-- Depends on gstreamer, which can be found at: http://gstreamer.freedesktop.org/download/
#

LinuxBuild {
    CONFIG += link_pkgconfig
    packagesExist(gstreamer-1.0) {
        PKGCONFIG   += gstreamer-1.0  gstreamer-video-1.0
        CONFIG      += VideoEnabled
    }
} else:MacBuild {
    #- gstreamer framework installed by the gstreamer devel installer
    GST_ROOT = /Library/Frameworks/GStreamer.framework
    exists($$GST_ROOT) {
        CONFIG      += VideoEnabled
        INCLUDEPATH += $$GST_ROOT/Headers
        LIBS        += -F/Library/Frameworks -framework GStreamer
    }
} else:iOSBuild {
    #- gstreamer framework installed by the gstreamer iOS SDK installer (default to home directory)
    GST_ROOT = $$(HOME)/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework
    exists($$GST_ROOT) {
        CONFIG      += VideoEnabled
        INCLUDEPATH += $$GST_ROOT/Headers
        LIBS        += -F$$(HOME)/Library/Developer/GStreamer/iPhone.sdk -framework GStreamer -liconv -lresolv
    }
} else:WindowsBuild {
    #- gstreamer installed by default under c:/gstreamer
    GST_ROOT = c:/gstreamer/1.0/x86
    exists($$GST_ROOT) {
        CONFIG      += VideoEnabled
        LIBS        += -L$$GST_ROOT/lib/gstreamer-1.0/static -lgstreamer-1.0 -lgstvideo-1.0 -lgstbase-1.0
        LIBS        += -L$$GST_ROOT/lib -lglib-2.0 -lintl -lgobject-2.0
        INCLUDEPATH += \
            $$GST_ROOT/include/gstreamer-1.0 \
            $$GST_ROOT/include/glib-2.0 \
            $$GST_ROOT/lib/gstreamer-1.0/include \
            $$GST_ROOT/lib/glib-2.0/include
    }
} else:AndroidBuild {
    #- gstreamer assumed to be installed in $$PWD/../../android/gstreamer-1.0-android-armv7-1.5.2
    GST_ROOT = $$PWD/../../gstreamer-1.0-android-armv7-1.5.2
    exists($$GST_ROOT) {
        QMAKE_CXXFLAGS  += -pthread
        CONFIG          += VideoEnabled

        # We want to link these plugins statically
        LIBS += -L$$GST_ROOT/lib/gstreamer-1.0/static \
            -lgstvideo-1.0 \
            -lgstcoreelements \
            -lgstudp \
            -lgstrtp \
            -lgstx264 \
            -lgstlibav \
            -lgstvideoparsersbad

        # Rest of GStreamer dependencies
        LIBS += -L$$GST_ROOT/lib \
            -lgstfft-1.0 -lm  \
            -lgstnet-1.0 -lgio-2.0 \
            -lgstaudio-1.0 -lgstcodecparsers-1.0 -lgstbase-1.0 \
            -lgstreamer-1.0 -lgsttag-1.0 -lgstrtp-1.0 -lgstpbutils-1.0 \
            -lgstvideo-1.0 -lavformat -lavcodec -lavresample -lavutil -lx264 \
            -lbz2 -lgobject-2.0 \
            -Wl,--export-dynamic -lgmodule-2.0 -pthread -lglib-2.0 -lorc-0.4 -liconv -lffi -lintl

        INCLUDEPATH += \
            $$GST_ROOT/include/gstreamer-1.0 \
            $$GST_ROOT/lib/gstreamer-1.0/include \
            $$GST_ROOT/include/glib-2.0 \
            $$GST_ROOT/lib/glib-2.0/include
    }
}

VideoEnabled {

    message("Including support for video streaming")

    DEFINES += \
        QGC_GST_STREAMING \
        GST_PLUGIN_BUILD_STATIC \
        QTGLVIDEOSINK_NAME=qt5glvideosink \
        QGC_VIDEOSINK_PLUGIN=qt5videosink

    INCLUDEPATH += \
        $$PWD/gstqtvideosink \
        $$PWD/gstqtvideosink/delegates \
        $$PWD/gstqtvideosink/painters \
        $$PWD/gstqtvideosink/utils \

    #-- QtGstreamer (gutted to our needs)

    HEADERS += \
        $$PWD/gstqtvideosink/delegates/basedelegate.h \
        $$PWD/gstqtvideosink/delegates/qtquick2videosinkdelegate.h \
        $$PWD/gstqtvideosink/delegates/qtvideosinkdelegate.h \
        $$PWD/gstqtvideosink/delegates/qwidgetvideosinkdelegate.h \
        $$PWD/gstqtvideosink/gstqtglvideosink.h \
        $$PWD/gstqtvideosink/gstqtglvideosinkbase.h \
        $$PWD/gstqtvideosink/gstqtquick2videosink.h \
        $$PWD/gstqtvideosink/gstqtvideosink.h \
        $$PWD/gstqtvideosink/gstqtvideosinkbase.h \
        $$PWD/gstqtvideosink/gstqtvideosinkmarshal.h \
        $$PWD/gstqtvideosink/gstqtvideosinkplugin.h \
        $$PWD/gstqtvideosink/gstqwidgetvideosink.h \
        $$PWD/gstqtvideosink/painters/abstractsurfacepainter.h \
        $$PWD/gstqtvideosink/painters/genericsurfacepainter.h \
        $$PWD/gstqtvideosink/painters/openglsurfacepainter.h \
        $$PWD/gstqtvideosink/painters/videomaterial.h \
        $$PWD/gstqtvideosink/painters/videonode.h \
        $$PWD/gstqtvideosink/utils/bufferformat.h \
        $$PWD/gstqtvideosink/utils/utils.h \
        $$PWD/gstqtvideosink/utils/glutils.h \

    SOURCES += \
        $$PWD/gstqtvideosink/delegates/basedelegate.cpp \
        $$PWD/gstqtvideosink/delegates/qtquick2videosinkdelegate.cpp \
        $$PWD/gstqtvideosink/delegates/qtvideosinkdelegate.cpp \
        $$PWD/gstqtvideosink/delegates/qwidgetvideosinkdelegate.cpp \
        $$PWD/gstqtvideosink/gstqtglvideosink.cpp \
        $$PWD/gstqtvideosink/gstqtglvideosinkbase.cpp \
        $$PWD/gstqtvideosink/gstqtvideosinkmarshal.c \
        $$PWD/gstqtvideosink/gstqtquick2videosink.cpp \
        $$PWD/gstqtvideosink/gstqtvideosink.cpp \
        $$PWD/gstqtvideosink/gstqtvideosinkbase.cpp \
        $$PWD/gstqtvideosink/gstqtvideosinkplugin.cpp \
        $$PWD/gstqtvideosink/gstqwidgetvideosink.cpp \
        $$PWD/gstqtvideosink/painters/genericsurfacepainter.cpp \
        $$PWD/gstqtvideosink/painters/openglsurfacepainter.cpp \
        $$PWD/gstqtvideosink/painters/videomaterial.cpp \
        $$PWD/gstqtvideosink/painters/videonode.cpp \
        $$PWD/gstqtvideosink/utils/bufferformat.cpp \
        $$PWD/gstqtvideosink/utils/utils.cpp \

} else {
    LinuxBuild|MacBuild|iOSBuild|WindowsBuild|AndroidBuild {
        message("Skipping support for video streaming (GStreamer libraries not installed)")
        MacBuild {
            message("  You can download it from http://gstreamer.freedesktop.org/data/pkg/osx/")
            message("  Select the devel package and install it (gstreamer-1.0-devel-1.x.x-x86_64.pkg)")
            message("  It will be installed in /Libraries/Frameworks")
        }
        LinuxBuild {
            message("  You can install it using apt-get")
            message("  sudo apt-get install gstreamer1.0*")
            message("  sudo apt-get install libgstreamer1.0*")
        }
        WindowsBuild {
            message("  You can download it from http://gstreamer.freedesktop.org/data/pkg/windows/")
            message("  Select the devel AND runtime packages and install them (x86, not the 64-Bit)")
            message("  It will be installed in C:/gstreamer. You need to update you PATH to point to the bin directory.")
        }
        AndroidBuild {
            message("  You can download it from http://gstreamer.freedesktop.org/data/pkg/android/")
            message("  Uncompress the archive into the qgc root source directory (same directory where qgroundcontrol.pro is found.")
        }
    } else {
        message("Skipping support for video streaming (Unsupported platform)")
    }
}

