# Video streaming application for simple UDP direct byte streaming


QT       += svg network opengl

TEMPLATE = app
TARGET = qgcvideo

BASEDIR = .
BUILDDIR = build/qgcvideo
LANGUAGE = C++

CONFIG += release
CONFIG -= debug

OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated

macx:DESTDIR = $$BASEDIR/bin/mac

INCLUDEPATH += . \
    src \
    src/ui \
    src/comm \
    include/ui \
    src/apps/qgcvideo \

# Input

HEADERS += \
    src/comm/UDPLink.h \
    src/comm/LinkInterface.h \
    src/comm/LinkManager.h \
    src/QGC.h \
    src/apps/qgcvideo/QGCVideoMainWindow.h \
    src/apps/qgcvideo/QGCVideoApp.h \
    src/apps/qgcvideo/QGCVideoWidget.h

SOURCES += \
    src/comm/UDPLink.cc \
    src/comm/LinkManager.cc \
    src/QGC.cc \
    src/apps/qgcvideo/main.cc \
    src/apps/qgcvideo/QGCVideoMainWindow.cc \
    src/apps/qgcvideo/QGCVideoApp.cc \
    src/apps/qgcvideo/QGCVideoWidget.cc

FORMS += \
    src/apps/qgcvideo/QGCVideoMainWindow.ui

RESOURCES = mavground.qrc
