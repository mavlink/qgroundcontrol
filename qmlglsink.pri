LinuxBuild {
    DEFINES += HAVE_QT_X11 HAVE_QT_EGLFS HAVE_QT_WAYLAND
} else:MacBuild {
    DEFINES += HAVE_QT_MAC
} else:iOSBuild {
    DEFINES += HAVE_QT_IOS
} else:WindowsBuild {
    DEFINES += HAVE_QT_WIN32 HAVE_QT_QPA_HEADER
    LIBS += opengl32.lib user32.lib
} else:AndroidBuild {
    DEFINES += HAVE_QT_ANDROID
}

SOURCES += \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstplugin.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqsgtexture.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtelement.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtglutility.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtoverlay.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsink.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsrc.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtglrenderer.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtitem.cc \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtwindow.cc

HEADERS += \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqsgtexture.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtelements.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtgl.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtglutility.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtoverlay.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsink.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsrc.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtglrenderer.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtitem.h \
    libs/qmlglsink/gstreamer/subprojects/gst-plugins-good/ext/qt/qtwindow.h
