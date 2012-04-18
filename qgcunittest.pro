#-------------------------------------------------
#
# Project created by QtCreator 2011-01-11T08:54:37
#
#-------------------------------------------------

QT += network \
    phonon \
    testlib \
    svg

TEMPLATE = app

TARGET = qgcunittest

BASEDIR = $$IN_PWD
TESTDIR = $$BASEDIR/qgcunittest
TARGETDIR = $$OUT_PWD
BUILDDIR = $$TARGETDIR/build
LANGUAGE = C++

CONFIG = qt thread console

OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated
MAVLINK_CONF = ""
MAVLINKPATH = $$BASEDIR/mavlink/include/v1.0
DEFINES += MAVLINK_NO_DATA

win32 {
    QMAKE_INCDIR_QT = $$(QTDIR)/include
    QMAKE_LIBDIR_QT = $$(QTDIR)/lib
    QMAKE_UIC = "$$(QTDIR)/bin/uic.exe"
    QMAKE_MOC = "$$(QTDIR)/bin/moc.exe"
    QMAKE_RCC = "$$(QTDIR)/bin/rcc.exe"
    QMAKE_QMAKE = "$$(QTDIR)/bin/qmake.exe"
}

# EIGEN matrix library (header-only)
INCLUDEPATH += src/libs/eigen

# If the user config file exists, it will be included.
# if the variable MAVLINK_CONF contains the name of an
# additional project, QGroundControl includes the support
# of custom MAVLink messages of this project
exists(user_config.pri) {
    include(user_config.pri)
    message("----- USING CUSTOM USER QGROUNDCONTROL CONFIG FROM user_config.pri -----")
    message("Adding support for additional MAVLink messages for: " $$MAVLINK_CONF)
    message("------------------------------------------------------------------------")
}

INCLUDEPATH += $$MAVLINKPATH/common
INCLUDEPATH += $$MAVLINKPATH
contains(MAVLINK_CONF, pixhawk) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common

    # PIXHAWK SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/pixhawk
    DEFINES += QGC_USE_PIXHAWK_MESSAGES
}
contains(MAVLINK_CONF, slugs) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common

    # SLUGS SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/slugs
    DEFINES += QGC_USE_SLUGS_MESSAGES
}
contains(MAVLINK_CONF, ualberta) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common

    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/ualberta
    DEFINES += QGC_USE_UALBERTA_MESSAGES
}
contains(MAVLINK_CONF, ardupilotmega) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common

    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/ardupilotmega
    DEFINES += QGC_USE_ARDUPILOTMEGA_MESSAGES
}

# Include general settings for QGroundControl
# necessary as last include to override any non-acceptable settings
# done by the plugins above
include(qgroundcontrol.pri)
# Reset QMAKE_POST_LINK to prevent file copy operations
QMAKE_POST_LINK = ""

# Include QWT plotting library
include(src/lib/qwt/qwt.pri)
DEPENDPATH += . \
    plugins \
    thirdParty/qserialport/include \
    thirdParty/qserialport/include/QtSerialPort \
    thirdParty/qserialport \
    src/libs/qextserialport

INCLUDEPATH += . \
    thirdParty/qserialport/include \
    thirdParty/qserialport/include/QtSerialPort \
    thirdParty/qserialport/src \
    src/libs/qextserialport

# QWT plot and QExtSerial depend on paths set by qgroundcontrol.pri
# Include serial port library
include(qserialport.pri)

# Serial port detection (ripped-off from qextserialport library)
macx|macx-g++|macx-g++42::SOURCES += src/libs/qextserialport/qextserialenumerator_osx.cpp
linux-g++::SOURCES += src/libs/qextserialport/qextserialenumerator_unix.cpp
linux-g++-64::SOURCES += src/libs/qextserialport/qextserialenumerator_unix.cpp
win32::SOURCES += src/libs/qextserialport/qextserialenumerator_win.cpp
win32-msvc2008|win32-msvc2010::SOURCES += src/libs/qextserialport/qextserialenumerator_win.cpp

SOURCES += src/uas/UAS.cc \
    src/comm/MAVLinkProtocol.cc \
    src/uas/UASWaypointManager.cc \
    src/Waypoint.cc \
    src/ui/RadioCalibration/RadioCalibrationData.cc \
    src/uas/SlugsMAV.cc \
    src/uas/PxQuadMAV.cc \
    src/uas/ArduPilotMegaMAV.cc \
    src/GAudioOutput.cc \
    src/uas/UASManager.cc \
    src/comm/LinkManager.cc \
    src/QGC.cc \
    src/comm/SerialLink.cc \
    $$TESTDIR/SlugsMavUnitTest.cc \
    $$TESTDIR/testSuite.cc \
    src/uas/QGCMAVLinkUASFactory.cc \
    $$TESTDIR/UASUnitTest.cc

INCLUDEPATH += src \
    src/ui \
    src/ui/linechart \
    src/ui/uas \
    src/ui/map \
    src/uas \
    src/comm \
    src/input \
    src/ui/mavlink \
    src/ui/watchdog \
    src/ui/map3D \
    src/ui/designer

HEADERS += src/uas/UASInterface.h \
    src/uas/UAS.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/ProtocolInterface.h \
    src/uas/UASWaypointManager.h \
    src/Waypoint.h \
    src/ui/RadioCalibration/RadioCalibrationData.h \
    src/uas/SlugsMAV.h \
    src/uas/PxQuadMAV.h \
    src/uas/ArduPilotMegaMAV.h \
    src/GAudioOutput.h \
    src/uas/UASManager.h \
    src/comm/LinkManager.h \
    src/comm/LinkInterface.h \
    src/QGC.h \
    src/comm/SerialLinkInterface.h \
    src/comm/SerialLink.h \
    src/configuration.h \
    $$TESTDIR/SlugsMavUnitTest.h \
    $$TESTDIR/AutoTest.h \
    $$TESTDIR/UASUnitTest.h \
    src/uas/QGCMAVLinkUASFactory.h


DEFINES += 'SRCDIR="$${PWD}/"'
