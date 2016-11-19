message("Adding Yuneec specific settings")

CONFIG  += NoSerialBuild
CONFIG  += MobileBuild

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += MINIMALIST_BUILD
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

SOURCES += \
    $$PWD/Yuneec/QGCCustom.cc \
    $$PWD/Yuneec/SerialComm.cc

HEADERS += \
    $$PWD/Yuneec/m4Defines.h \
    $$PWD/Yuneec/QGCCustom.h \
    $$PWD/Yuneec/SerialComm.h

INCLUDEPATH += \
    $$PWD/Yuneec

Androidx86Build {
    include($$PWD/AndroidYuneec.pri)
} else {
    error("Bulding the Yuneec version is only supported for Android x86 builds")
}
