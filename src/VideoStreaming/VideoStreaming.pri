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
        message("Including support for video streaming")
        PKGCONFIG   += gstreamer-1.0  gstreamer-video-1.0
        CONFIG      += VideoEnabled
    }
} else:MacBuild {
    #- gstreamer framework installed by the gstreamer devel installer
    GST_ROOT = /Library/Frameworks/GStreamer.framework
    exists($$GST_ROOT) {
        message("Including support for video streaming")
        CONFIG      += VideoEnabled
        INCLUDEPATH += $$GST_ROOT/Headers
        LIBS        += -F/Library/Frameworks -framework GStreamer
    }
} else:iOSBuild {
    #- gstreamer framework installed by the gstreamer iOS SDK installer (default to home directory)
    GST_ROOT = $$(HOME)/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework
    exists($$GST_ROOT) {
        message("Including support for video streaming")
        CONFIG      += VideoEnabled
        INCLUDEPATH += $$GST_ROOT/Headers
        LIBS        += -F$$(HOME)/Library/Developer/GStreamer/iPhone.sdk -framework GStreamer
    }
} else:WindowsBuild {
    #- gstreamer installed by default under c:/gstreamer
    #  This is not working. It builds but at runtime, it expects to find
    #  the gstreamer DLLs, even though it is, in theory, linking to it
    #  statically.
    exists(foobar_removethistobuild) {
        GST_ROOT = c:/gstreamer/1.0/x86
        exists($$GST_ROOT) {
            message("Including support for video streaming")
            CONFIG      += VideoEnabled
            LIBS        += -L$$GST_ROOT/lib/gstreamer-1.0/static -lgstreamer-1.0 -lgstvideo-1.0 -lgstbase-1.0
            LIBS        += -L$$GST_ROOT/lib -lglib-2.0 -lintl -lgobject-2.0
            INCLUDEPATH += \
                $$GST_ROOT/include/gstreamer-1.0 \
                $$GST_ROOT/include/glib-2.0 \
                $$GST_ROOT/lib/gstreamer-1.0\include \
                $$GST_ROOT/lib/glib-2.0/include
        }
    }
}

VideoEnabled {

    DEFINES += \
        QGC_GST_STREAMING \
        GST_PLUGIN_BUILD_STATIC \
        QTGLVIDEOSINK_NAME=qt5glvideosink \
        QTVIDEOSINK_NAME=qt5videosink

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
    message("Skipping support for video streaming (Unsupported platform)")
}

