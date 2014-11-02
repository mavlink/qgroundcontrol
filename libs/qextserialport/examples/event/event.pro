TEMPLATE = app
DEPENDPATH += .
CONFIG += console
include(../../src/qextserialport.pri)

SOURCES += main.cpp PortListener.cpp
HEADERS += PortListener.h
