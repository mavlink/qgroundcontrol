android {

    QT += androidextras core-private

    INCLUDEPATH += $$PWD

    PUBLIC_HEADERS += \
        $$PWD/qserialport.h \
        $$PWD/qserialportinfo.h

    PRIVATE_HEADERS += \
        $$PWD/qserialport_p.h \
        $$PWD/qserialportinfo_p.h \
        $$PWD/qserialport_android_p.h

    SOURCES += \
        $$PWD/qserialport.cpp \
        $$PWD/qserialportinfo.cpp \
        $$PWD/qserialport_android.cpp \
        $$PWD/qserialportinfo_android.cpp

    CONFIG += mobility

    HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

}
