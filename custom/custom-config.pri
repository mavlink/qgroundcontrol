CONFIG  += NoSerialBuild
CONFIG  += MobileBuild
CONFIG  += DISABLE_BUILTIN_ANDROID

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

PLUGIN_DESTDIR = /tmp
PLUGIN_TARGET  = TyphoonH.core
PLUGIN_SOURCE  = $${PLUGIN_DESTDIR}/lib$${PLUGIN_TARGET}.so

#-------------------------------------------------------------------------------------
# Android

Androidx86Build {
    ANDROID_EXTRA_LIBS += $${PLUGIN_SOURCE}
    include($$PWD/AndroidTyphoonH.pri)
}
