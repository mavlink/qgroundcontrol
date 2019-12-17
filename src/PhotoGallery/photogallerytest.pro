# This build file defines hermetic unit tests for photo gallery related
# features. The build pulls in only those sources that are required in the
# context of the test cases, and is independent of the bulk of QGroundControl.

QT_CONFIG -= no-pkg-config
PKGCONFIG += libexif
CONFIG += testcase link_pkgconfig
QT += testlib xml
TARGET = PhotoGalleryTests
SOURCES = \
    PhotoGalleryTests.cc

include($$PWD/photogallery_hermetic.pri)
