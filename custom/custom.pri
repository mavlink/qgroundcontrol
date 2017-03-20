message("Adding Yuneec Typhoon H plugin")

linux : android-g++ {
    CONFIG += DISABLE_BUILTIN_ANDROID
    equals(ANDROID_TARGET_ARCH, x86)  {
        message("Using Typhoon specific Android interfaces")
    } else {
        message("Unsuported Android toolchain, limited functionality for development only")
    }
} else {
    message("Non suported Platform, limited functionality for development only")
    DEFINES += __mobile__
}

DEFINES += CUSTOMHEADER=\"\\\"TyphoonHPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=TyphoonHPlugin

CONFIG  += NoSerialBuild
CONFIG  += MobileBuild
CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY QGC_DISABLE_PX4_PLUGIN_FACTORY
CONFIG  += DISABLE_VIDEORECORDING

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

RESOURCES += \
    $$QGCROOT/custom/typhoonh.qrc

SOURCES += \
    $$PWD/src/CameraControl.cc \
    $$PWD/src/m4serial.cc \
    $$PWD/src/m4util.cc \
    $$PWD/src/TyphoonHM4Interface.cc \
    $$PWD/src/TyphoonHPlugin.cc \
    $$PWD/src/TyphoonHQuickInterface.cc \

AndroidBuild {
    SOURCES += \
        $$PWD/src/TyphoonHJNI.cc \
}

HEADERS += \
    $$PWD/src/CameraControl.h \
    $$PWD/src/m4channeldata.h \
    $$PWD/src/m4def.h \
    $$PWD/src/m4serial.h \
    $$PWD/src/m4util.h \
    $$PWD/src/TyphoonHCommon.h \
    $$PWD/src/TyphoonHM4Interface.h \
    $$PWD/src/TyphoonHPlugin.h \
    $$PWD/src/TyphoonHQuickInterface.h \

INCLUDEPATH += \
    $$PWD/src \

equals(QT_MAJOR_VERSION, 5) {
    greaterThan(QT_MINOR_VERSION, 5) {
        ReleaseBuild {
            QT      += qml-private
            CONFIG  += qtquickcompiler
            message("Using Qt Quick Compiler")
        }
    }
}

QT += \
    multimedia

#-------------------------------------------------------------------------------------
# Firmware/AutoPilot Plugin

INCLUDEPATH += \
    $$PWD/src/FirmwarePlugin \
    $$PWD/src/AutoPilotPlugin

HEADERS+= \
    $$PWD/src/AutoPilotPlugin/YuneecAutoPilotPlugin.h \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePlugin.h \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePluginFactory.h

SOURCES += \
    $$PWD/src/AutoPilotPlugin/YuneecAutoPilotPlugin.cc \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePlugin.cc \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePluginFactory.cc

#-------------------------------------------------------------------------------------
# Android

AndroidBuild {
    ANDROID_EXTRA_LIBS += $${PLUGIN_SOURCE}
    include($$PWD/AndroidTyphoonH.pri)
}
