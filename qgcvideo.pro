# Video streaming application for simple UDP direct byte streaming


QT       += svg network opengl

TEMPLATE = app
TARGET = qgcvideo

BASEDIR = .

LANGUAGE = C++




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
