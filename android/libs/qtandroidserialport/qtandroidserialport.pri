QT += core core-private

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/qserialport.cpp \
    $$PWD/qserialportinfo.cpp \
    $$PWD/qserialport_android.cpp \
    $$PWD/qserialportinfo_android.cpp

HEADERS += \
    $$PWD/qserialport.h \
    $$PWD/qserialportinfo.h \
    $$PWD/qserialport_p.h \
    $$PWD/qserialportinfo_p.h \
    $$PWD/qserialport_android_p.h
