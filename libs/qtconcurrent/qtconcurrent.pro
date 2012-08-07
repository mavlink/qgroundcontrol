TEMPLATE = lib
TARGET = QtConcurrent
DEFINES += BUILD_QTCONCURRENT

include(../../openpilotgcslibrary.pri)

HEADERS += \
    qtconcurrent_global.h \
    multitask.h \
    runextensions.h
