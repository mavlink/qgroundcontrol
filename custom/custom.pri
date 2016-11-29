message("Adding Yuneec Typhoon H plugin")

linux : android-g++ {
    !equals(ANDROID_TARGET_ARCH, x86)  {
        error("Unsuported Android toolchain, only x86 supported")
    }
} else {
    message("Non suported Platform, limited functionality development only")
    DEFINES += __mobile__
    CONFIG  += DISABLE_VIDEOSTREAMING
}

DEFINES += CUSTOMHEADER=\"\\\"typhoonh.h\\\"\"
DEFINES += CUSTOMCLASS=TyphoonHPlugin

CONFIG  += NoSerialBuild
CONFIG  += MobileBuild
CONFIG  += DISABLE_BUILTIN_ANDROID QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

RESOURCES += \
    $$PWD/typhoonh.qrc \

SOURCES += \
    $$PWD/src/m4.cc \
    $$PWD/src/m4serial.cc \
    $$PWD/src/m4util.cc \
    $$PWD/src/typhoonh.cc \

HEADERS += \
    $$PWD/src/m4.h \
    $$PWD/src/m4channeldata.h \
    $$PWD/src/m4def.h \
    $$PWD/src/m4serial.h \
    $$PWD/src/m4util.h \
    $$PWD/src/typhoonh.h \

INCLUDEPATH += \
    $$PWD/src \

#-------------------------------------------------------------------------------------
# Android

Androidx86Build {
    ANDROID_EXTRA_LIBS += $${PLUGIN_SOURCE}
    include($$PWD/AndroidTyphoonH.pri)
}
