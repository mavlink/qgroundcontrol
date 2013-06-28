INCLUDEPATH += $$PWD

unix {
    CONFIG += link_pkgconfig

    packagesExist(libudev) {
        DEFINES += HAVE_LIBUDEV
        PKGCONFIG += libudev
    }
}

PUBLIC_HEADERS += \
    $$PWD/qserialportglobal.h \
    $$PWD/qserialport.h \
    $$PWD/qserialportinfo.h

PRIVATE_HEADERS += \
    $$PWD/qserialport_p.h \
    $$PWD/qserialportinfo_p.h

SOURCES += \
    $$PWD/qserialport.cpp \
    $$PWD/qserialportinfo.cpp

win32 {
    PRIVATE_HEADERS += \
        $$PWD/qserialport_win_p.h

    SOURCES += \
        $$PWD/qserialport_win.cpp \
        $$PWD/qserialportinfo_win.cpp

    !wince*: {
        LIBS += -lsetupapi -ladvapi32
    } else {
        SOURCES += \
            $$PWD/qserialport_wince.cpp \
            $$PWD/qserialportinfo_wince.cpp
    }
}

symbian {
    MMP_RULES += EXPORTUNFROZEN
    #MMP_RULES += DEBUGGABLE_UDEBONLY
    TARGET.UID3 = 0xE7E62DFD
    TARGET.CAPABILITY =
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = QtSerialPort.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles

    # FIXME !!!
    #INCLUDEPATH += c:/Nokia/devices/Nokia_Symbian3_SDK_v1.0/epoc32/include/platform
    INCLUDEPATH += c:/QtSDK/Symbian/SDKs/Symbian3Qt473/epoc32/include/platform

    PRIVATE_HEADERS += \
        $$PWD/qserialport_symbian_p.h

    SOURCES += \
        $$PWD/qserialport_symbian.cpp \
        $$PWD/qserialportinfo_symbian.cpp

    LIBS += -leuser -lefsrv -lc32
}

unix:!symbian {
    PRIVATE_HEADERS += \
        $$PWD/qttylocker_unix_p.h \
        $$PWD/qserialport_unix_p.h

    SOURCES += \
        $$PWD/qttylocker_unix.cpp \
        $$PWD/qserialport_unix.cpp \
        $$PWD/qserialportinfo_unix.cpp

    macx {
        SOURCES += $$PWD/qserialportinfo_mac.cpp

        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        linux*:contains( DEFINES, HAVE_LIBUDEV ) {
            LIBS += -ludev
        }
    }
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
