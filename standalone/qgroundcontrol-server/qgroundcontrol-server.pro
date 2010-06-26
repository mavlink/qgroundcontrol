QT       += net

TEMPLATE = app
TARGET = qgroundcontrol-server

BASEDIR = ../..
BUILDDIR = $$BASEDIR/build/qgroundcontrol-server
LANGUAGE = C++

CONFIG += release
CONFIG -= debug

OBJECTS_DIR = $$BUILDDIR/qgroundcontrol-server/obj
MOC_DIR = $$BUILDDIR/qgroundcontrol-server/moc

macx:DESTDIR = $$BASEDIR/bin/mac

INCLUDEPATH += $$BASEDIR/. \
    $$BASEDIR/src \
    $$BASEDIR/src/comm \
    $$BASEDIR/standalone/qgroundcontrol-server/src

HEADERS += src/QGroundControlServer.h \
   $$BASEDIR/src/comm/MAVLinkProtocol.h
SOURCES += src/main.cc \
	src/QGroundControlServer.cc \
	$$BASEDIR/src/comm/MAVLinkProtocol.cc
RESOURCES = $$BASEDIR/mavground.qrc
