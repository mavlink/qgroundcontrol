TEMPLATE = app
DEPENDPATH += .
QT += core gui
contains(QT_VERSION, ^5\\..*\\..*): QT += widgets
HEADERS += MainWindow.h \
        MessageWindow.h \
        QespTest.h

SOURCES += main.cpp \
           MainWindow.cpp \
           MessageWindow.cpp \
           QespTest.cpp

include(../../src/qextserialport.pri)
