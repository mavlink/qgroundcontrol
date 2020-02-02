################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

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
    GST_ROOT = c:/gstreamer/1.0/x86_64
    exists($$GST_ROOT) {
        CONFIG      += VideoEnabled

        LIBS        += -L$$GST_ROOT/lib -lgstreamer-1.0 -lgstvideo-1.0 -lgstbase-1.0
        LIBS        += -lglib-2.0 -lintl -lgobject-2.0

        INCLUDEPATH += \
            $$GST_ROOT/include/gstreamer-1.0 \
            $$GST_ROOT/include/glib-2.0 \
            $$GST_ROOT/lib/gstreamer-1.0/include \
            $$GST_ROOT/lib/glib-2.0/include

        DESTDIR_WIN = $$replace(DESTDIR, "/", "\\")
        GST_ROOT_WIN = $$replace(GST_ROOT, "/", "\\")

        # Copy main GStreamer runtime files
        QMAKE_POST_LINK += $$escape_expand(\\n) xcopy \"$$GST_ROOT_WIN\\bin\*.dll\" \"$$DESTDIR_WIN\" /S/Y $$escape_expand(\\n)
        QMAKE_POST_LINK += xcopy \"$$GST_ROOT_WIN\\bin\*.\" \"$$DESTDIR_WIN\" /S/Y $$escape_expand(\\n)

        # Copy GStreamer plugins
        QMAKE_POST_LINK += $$escape_expand(\\n) xcopy \"$$GST_ROOT_WIN\\lib\\gstreamer-1.0\\*.dll\" \"$$DESTDIR_WIN\\gstreamer-plugins\\\" /Y $$escape_expand(\\n)
        QMAKE_POST_LINK += $$escape_expand(\\n) xcopy \"$$GST_ROOT_WIN\\lib\\gstreamer-1.0\\validate\\*.dll\" \"$$DESTDIR_WIN\\gstreamer-plugins\\validate\\\" /Y $$escape_expand(\\n)
    }
} else:AndroidBuild {
    #- gstreamer assumed to be installed in $$PWD/../../gstreamer-1.0-android-universal-1.14.4/***
    contains(QT_ARCH, arm) {
        GST_ROOT = $$PWD/../../gstreamer-1.0-android-universal-1.14.4/armv7
    } else:contains(QT_ARCH, arm64) {
        GST_ROOT = $$PWD/../../gstreamer-1.0-android-universal-1.14.4/arm64
    } else {
        GST_ROOT = $$PWD/../../gstreamer-1.0-android-universal-1.14.4/x86
    }
    exists($$GST_ROOT) {
        QMAKE_CXXFLAGS  += -pthread
        CONFIG          += VideoEnabled

        # We want to link these plugins statically
        LIBS += -L$$GST_ROOT/lib/gstreamer-1.0 \
            -lgstvideo-1.0 \
            -lgstcoreelements \
            -lgstudp \
            -lgstrtp \
            -lgstrtsp \
            -lgstx264 \
            -lgstlibav \
            -lgstsdpelem \
            -lgstvideoparsersbad \
            -lgstrtpmanager \
            -lgstisomp4 \
            -lgstmatroska \
            -lgstandroidmedia

        # Rest of GStreamer dependencies
        LIBS += -L$$GST_ROOT/lib \
            -lgstfft-1.0 -lm  \
            -lgstnet-1.0 -lgio-2.0 \
            -lgstphotography-1.0 -lgstgl-1.0 -lEGL \
            -lgstaudio-1.0 -lgstcodecparsers-1.0 -lgstbase-1.0 \
            -lgstreamer-1.0 -lgstrtp-1.0 -lgstpbutils-1.0 -lgstrtsp-1.0 -lgsttag-1.0 \
            -lgstvideo-1.0 -lavformat -lavcodec -lavutil -lx264 -lavfilter -lswresample \
            -lgstriff-1.0 -lgstcontroller-1.0 -lgstapp-1.0 \
            -lgstsdp-1.0 -lbz2 -lgobject-2.0 \
            -Wl,--export-dynamic -lgmodule-2.0 -pthread -lglib-2.0 -lorc-0.4 -liconv -lffi -lintl \

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

    iOSBuild {
        OBJECTIVE_SOURCES += \
            $$PWD/ios/gst_ios_init.m
        INCLUDEPATH += \
            $$PWD/ios
    }

} else {
    LinuxBuild|MacBuild|iOSBuild|WindowsBuild|AndroidBuild {
        message("Skipping support for video streaming (GStreamer libraries not installed)")
        message("Installation instructions here: https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoStreaming/README.md")
    } else {
        message("Skipping support for video streaming (Unsupported platform)")
    }
}

