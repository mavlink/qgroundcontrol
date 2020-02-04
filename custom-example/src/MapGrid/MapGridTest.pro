# This build file defines hermetic unit tests for Map Grid related
# features. The build pulls in only those sources that are required in the
# context of the test cases, and is independent of the bulk of QGroundControl.

QT_CONFIG -= no-pkg-config
PKGCONFIG += libexif
CONFIG += testcase link_pkgconfig
QT += \
    testlib \
    location

TARGET = MapGridTests

SOURCES = \
    ../../../src/Geo/QGCGeo.cc \
    ../../../src/Geo/Math.cpp \
    ../../../src/Geo/Utility.cpp \
    ../../../src/Geo/UTMUPS.cpp \
    ../../../src/Geo/MGRS.cpp \
    ../../../src/Geo/TransverseMercator.cpp \
    ../../../src/Geo/PolarStereographic.cpp \
    MapGridMGRS.cc \
    MapGridTests.cc

HEADERS +=  \
    MapGridMGRS.h

INCLUDEPATH += \
    ../../../src/Geo
