android {

    QT += core-private

    INCLUDEPATH += $$PWD

    PUBLIC_HEADERS += \
        $$PWD/qserialport.h \
        $$PWD/qserialportinfo.h \
        $$PWD/qserialportglobal.h \
        $$PWD/qtserialportexports.h \
        $$PWD/qtserialportversion.h

    PRIVATE_HEADERS += \
        $$PWD/qserialport_p.h \
        $$PWD/qserialportinfo_p.h

    SOURCES += \
        $$PWD/qserialport.cpp \
        $$PWD/qserialportinfo.cpp \
        $$PWD/qserialport_android.cpp \
        $$PWD/qserialportinfo_android.cpp

    CONFIG += mobility

    HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

}
