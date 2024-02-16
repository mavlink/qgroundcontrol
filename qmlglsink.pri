LinuxBuild {
    DEFINES += HAVE_QT_WAYLAND HAVE_QT_X11 HAVE_QT_EGLFS
} else:MacBuild {
    DEFINES += HAVE_QT_MAC
} else:iOSBuild {
    DEFINES += HAVE_QT_IOS
} else:WindowsBuild {
    DEFINES += HAVE_QT_WIN32
    LIBS += opengl32.lib user32.lib
} else:AndroidBuild {
    DEFINES += HAVE_QT_ANDROID
}

DEFINES += HAVE_QT_QPA_HEADER QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>

LinuxBuild {
    SOURCES += \
        libs/qmlglsink/qt6-linux/gstplugin.cc \
        libs/qmlglsink/qt6-linux/gstqml6glsink.cc \
        libs/qmlglsink/qt6-linux/gstqsg6glnode.cc \
        libs/qmlglsink/qt6-linux/gstqt6element.cc \
        libs/qmlglsink/qt6-linux/gstqt6glutility.cc \
        libs/qmlglsink/qt6-linux/qt6glitem.cc

    HEADERS += \
        libs/qmlglsink/qt6-linux/gstqml6glsink.h \
        libs/qmlglsink/qt6-linux/gstqsg6glnode.h \
        libs/qmlglsink/qt6-linux/gstqt6elements.h \
        libs/qmlglsink/qt6-linux/gstqt6gl.h \
        libs/qmlglsink/qt6-linux/gstqt6glutility.h \
        libs/qmlglsink/qt6-linux/qt6glitem.h

    INCLUDEPATH += libs/qmlglsink/qt6-linux
} else {
    SOURCES += \
        libs/qmlglsink/qt6/gstplugin.cc \
        libs/qmlglsink/qt6/gstqml6glmixer.cc \
        libs/qmlglsink/qt6/gstqml6gloverlay.cc \
        libs/qmlglsink/qt6/gstqml6glsink.cc \
        libs/qmlglsink/qt6/gstqml6glsrc.cc \
        libs/qmlglsink/qt6/gstqsg6material.cc \
        libs/qmlglsink/qt6/gstqt6element.cc \
        libs/qmlglsink/qt6/gstqt6glutility.cc \
        libs/qmlglsink/qt6/qt6glitem.cc \
        libs/qmlglsink/qt6/qt6glrenderer.cc \
        libs/qmlglsink/qt6/qt6glwindow.cc

    HEADERS += \
        libs/qmlglsink/qt6/gstqml6glmixer.h \
        libs/qmlglsink/qt6/gstqml6gloverlay.h \
        libs/qmlglsink/qt6/gstqml6glsink.h \
        libs/qmlglsink/qt6/gstqml6glsrc.h \
        libs/qmlglsink/qt6/gstqsg6material.h \
        libs/qmlglsink/qt6/gstqt6elements.h \
        libs/qmlglsink/qt6/gstqt6gl.h \
        libs/qmlglsink/qt6/gstqt6glutility.h \
        libs/qmlglsink/qt6/qt6glitem.h \
        libs/qmlglsink/qt6/qt6glrenderer.h \
        libs/qmlglsink/qt6/qt6glwindow.h

    INCLUDEPATH += libs/qmlglsink/qt6
}
