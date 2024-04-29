LinuxBuild {
    UseWayland: {
        DEFINES += HAVE_QT_WAYLAND
    } else {
        DEFINES += HAVE_QT_X11
    }
    DEFINES += HAVE_QT_EGLFS HAVE_QT_QPA_HEADER
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
    libs/qmlglsink/qt6/gstplugin.cc \
    libs/qmlglsink/qt6/gstqml6glsink.cc \
    libs/qmlglsink/qt6/gstqsg6glnode.cc \
    libs/qmlglsink/qt6/gstqt6element.cc \
    libs/qmlglsink/qt6/gstqt6glutility.cc \
    libs/qmlglsink/qt6/qt6glitem.cc

HEADERS += \
    libs/qmlglsink/qt6/gstqml6glsink.h \
    libs/qmlglsink/qt6/gstqsg6glnode.h \
    libs/qmlglsink/qt6/gstqt6elements.h \
    libs/qmlglsink/qt6/gstqt6gl.h \
    libs/qmlglsink/qt6/gstqt6glutility.h \
    libs/qmlglsink/qt6/qt6glitem.h
