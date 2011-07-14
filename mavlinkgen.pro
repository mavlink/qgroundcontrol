
# }
# Include general settings for MAVGround
# necessary as last include to override any non-acceptable settings
# done by the plugins above
QT       += svg xml

TEMPLATE = app
TARGET = mavlinkgen

BASEDIR = .
BUILDDIR = build/mavlinkgen
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
    src/ui/mavlink \
    src/standalone/mavlinkgen

# Input
FORMS += src/ui/XMLCommProtocolWidget.ui

HEADERS += src/standalone/mavlinkgen/MAVLinkGen.h \
    src/ui/XMLCommProtocolWidget.h \
    src/comm/MAVLinkXMLParser.h \
    src/ui/mavlink/DomItem.h \
    src/ui/mavlink/DomModel.h \
    src/ui/mavlink/QGCMAVLinkTextEdit.h
SOURCES += src/standalone/mavlinkgen/main.cc \
    src/standalone/mavlinkgen/MAVLinkGen.cc \
    src/ui/XMLCommProtocolWidget.cc \
    src/ui/mavlink/DomItem.cc \
    src/ui/mavlink/DomModel.cc \
    src/comm/MAVLinkXMLParser.cc \
    src/ui/mavlink/QGCMAVLinkTextEdit.cc
RESOURCES = mavground.qrc
