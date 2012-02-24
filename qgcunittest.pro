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

CONFIG   += console
CONFIG   -= app_bundle

OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated
MAVLINK_CONF = ""

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

INCLUDEPATH += $$BASEDIR/../mavlink/include/common
contains(MAVLINK_CONF, pixhawk) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common

    # PIXHAWK SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/pixhawk
    DEFINES += QGC_USE_PIXHAWK_MESSAGES
}
contains(MAVLINK_CONF, slugs) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common

    # SLUGS SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/slugs
    DEFINES += QGC_USE_SLUGS_MESSAGES
}
contains(MAVLINK_CONF, ualberta) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common

    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/ualberta
    DEFINES += QGC_USE_UALBERTA_MESSAGES
}
contains(MAVLINK_CONF, ardupilotmega) {
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common

    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/ardupilotmega
    DEFINES += QGC_USE_ARDUPILOTMEGA_MESSAGES
}

# Include general settings for QGroundControl
# necessary as last include to override any non-acceptable settings
# done by the plugins above
include(qgroundcontrol.pri)
# Reset QMAKE_POST_LINK to prevent file copy operations
QMAKE_POST_LINK = ""

# QWT plot and QExtSerial depend on paths set by qgroundcontrol.pri
# Include serial port library
include(src/lib/qextserialport/qextserialport.pri)

# Include QWT plotting library
include(src/lib/qwt/qwt.pri)
DEPENDPATH += . \
    lib/QMapControl \
    lib/QMapControl/src \
    plugins
INCLUDEPATH += . \
    lib/QMapControl \
    $$BASEDIR/../mavlink/include \
    $$BASEDIR/src/uas \
    $$BASEDIR/src/comm \
    $$BASEDIR/src/ \
    $$BASEDIR/src/ui/RadioCalibration \
    $$BASEDIR/src/ui/ \


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
    $$TESTDIR/UASUnitTest.cc \
    src/uas/QGCMAVLinkUASFactory.cc


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
    $$TESTDIR//SlugsMavUnitTest.h \
    $$TESTDIR/AutoTest.h \
    $$TESTDIR/UASUnitTest.h \
    src/uas/QGCMAVLinkUASFactory.h


DEFINES += SRCDIR=\\\"$$PWD/\\\"
