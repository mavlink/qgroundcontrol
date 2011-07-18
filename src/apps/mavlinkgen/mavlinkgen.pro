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
SOURCES += main.cc \
    MAVLinkGen.cc