CONFIG  += NoSerialBuild
CONFIG  += MobileBuild
CONFIG  += DISABLE_BUILTIN_ANDROID

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += MINIMALIST_BUILD
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

#-------------------------------------------------------------------------------------
# Android

include($$PWD/AndroidTyphoonH.pri)
