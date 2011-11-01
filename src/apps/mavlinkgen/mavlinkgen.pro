# MAVLink code generator
# generates code in several languages for MAVLink encoding/decoding

QT       += svg xml

TEMPLATE = app
TARGET = mavlinkgen

LANGUAGE = C++

# Widget files (can be included in third-party Qt applications)
include(mavlinkgen.pri)

# Standalone files
HEADERS += MAVLinkGen.h
win32-msvc2008|win32-msvc2010 {
HEADERS += msinttypes/inttypes.h \
	msinttypes/stdint.h
INCLUDEPATH += msinttypes
}
SOURCES += main.cc \
    MAVLinkGen.cc
