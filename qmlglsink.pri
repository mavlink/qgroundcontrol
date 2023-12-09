LinuxBuild {
    DEFINES += HAVE_QT_X11 HAVE_QT_EGLFS HAVE_QT_WAYLAND HAVE_QT_QPA_HEADER
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
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstplugin.cc \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqml6glsink.cc \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqsg6glnode.cc \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqt6element.cc \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqt6glutility.cc \
    libs/qmlglsink/gst-plugins-good/ext/qt6/qt6glitem.cc 

HEADERS += \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqml6glsink.h \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqsg6glnode.h \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqt6elements.h \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqt6gl.h \
    libs/qmlglsink/gst-plugins-good/ext/qt6/gstqt6glutility.h \
    libs/qmlglsink/gst-plugins-good/ext/qt6/qt6glitem.h
