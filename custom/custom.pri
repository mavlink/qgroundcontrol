message("Adding Yuneec Typhoon H plugin")

linux : android-g++ {
    equals(ANDROID_TARGET_ARCH, x86)  {
        CONFIG  += DISABLE_BUILTIN_ANDROID
        message("Using Typhoon specific Android interfaces")
    } else {
        message("Unsuported Android toolchain, limited functionality for development only")
    }
} else {
    message("Non suported Platform, limited functionality for development only")
    DEFINES += __mobile__
}

DEFINES += CUSTOMHEADER=\"\\\"typhoonh.h\\\"\"
DEFINES += CUSTOMCLASS=TyphoonHPlugin

CONFIG  += NoSerialBuild
CONFIG  += MobileBuild
CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY QGC_DISABLE_PX4_PLUGIN QGC_DISABLE_PX4_PLUGIN_FACTORY
CONFIG  += DISABLE_VIDEORECORDING

DEFINES += NO_SERIAL_LINK
DEFINES += NO_UDP_VIDEO
DEFINES += QGC_DISABLE_BLUETOOTH
DEFINES += QGC_DISABLE_UVC
DEFINES += DISABLE_ZEROCONF

RESOURCES += \
    $$PWD/typhoonh.qrc \

SOURCES += \
    $$PWD/src/camera.cc \
    $$PWD/src/m4.cc \
    $$PWD/src/m4serial.cc \
    $$PWD/src/m4util.cc \
    $$PWD/src/typhoonh.cc \

HEADERS += \
    $$PWD/src/camera.h \
    $$PWD/src/m4.h \
    $$PWD/src/m4channeldata.h \
    $$PWD/src/m4def.h \
    $$PWD/src/m4serial.h \
    $$PWD/src/m4util.h \
    $$PWD/src/typhoonh.h \

INCLUDEPATH += \
    $$PWD/src \

ReleaseBuild {
    QT      += qml-private
    CONFIG  += qtquickcompiler
}

QT += \
    multimedia

#-------------------------------------------------------------------------------------
# Firmware Plugin

RESOURCES += $$QGCROOT/src/FirmwarePlugin/PX4/PX4Resources.qrc

INCLUDEPATH += \
    $$PWD/src/FirmwarePlugin \
    $$QGCROOT/src/AutoPilotPlugins/PX4 \
    $$QGCROOT/src/FirmwarePlugin/PX4 \

HEADERS+= \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponentAirframes.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponentController.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/CameraComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/FlightModesComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AirframeLoader.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4RadioComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4TuningComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PowerComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PowerComponentController.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SafetyComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SensorsComponent.h \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SensorsComponentController.h \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4GeoFenceManager.h \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4ParameterMetaData.h \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePlugin.h \

SOURCES += \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/AirframeComponentController.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/CameraComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/FlightModesComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4RadioComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PX4TuningComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PowerComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/PowerComponentController.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SafetyComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SensorsComponent.cc \
    $$QGCROOT/src/AutoPilotPlugins/PX4/SensorsComponentController.cc \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4GeoFenceManager.cc \
    $$QGCROOT/src/FirmwarePlugin/PX4/PX4ParameterMetaData.cc \
    $$PWD/src/FirmwarePlugin/YuneecFirmwarePlugin.cc \

HEADERS   += $$PWD/src/FirmwarePlugin/YuneecFirmwarePluginFactory.h
SOURCES   += $$PWD/src/FirmwarePlugin/YuneecFirmwarePluginFactory.cc

#-------------------------------------------------------------------------------------
# Android

Androidx86Build {
    ANDROID_EXTRA_LIBS += $${PLUGIN_SOURCE}
    include($$PWD/AndroidTyphoonH.pri)
}
