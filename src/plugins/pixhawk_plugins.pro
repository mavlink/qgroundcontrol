# This plugin is also identified by this line in PxMAV.cc
# Q_EXPORT_PLUGIN2(pixhawk_plugins, PxMAV)

include(../../qgroundcontrol.pri)

TEMPLATE      = lib
CONFIG       += plugin
QT += phonon
INCLUDEPATH  += ../. \
    ../../../mavlink/include \
    ../../MAVLink/include \
    ../../mavlink/include \
    ../uas \
    ../comm
HEADERS       = PxMAV.h
SOURCES       = PxMAV.cc \
    ../uas/UAS.cc \
    ../GAudioOutput.cc \
    ../comm/MAVLinkProtocol.cc \
    ../uas/UASManager.cc
TARGET        = $$qtLibraryTarget(pixhawk_plugins)
DESTDIR       = ../../plugins
