# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2015 QGroundControl Developers
# This file is part of the open groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

exists($${OUT_PWD}/qgroundcontrol.pro) {
    error("You must use shadow build (e.g. mkdir build; cd build; qmake ../qgroundcontrol.pro).")
}

message(Qt version $$[QT_VERSION])

!equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 4) {
    error("Unsupported Qt version, 5.5+ is required")
}

include(QGCCommon.pri)

TARGET   = qgroundcontrol
TEMPLATE = app

DebugBuild {
    DESTDIR  = $${OUT_PWD}/debug
} else {
    DESTDIR  = $${OUT_PWD}/release
}

# Load additional config flags from user_config.pri
exists(user_config.pri):infile(user_config.pri, CONFIG) {
    CONFIG += $$fromfile(user_config.pri, CONFIG)
    message($$sprintf("Using user-supplied additional config: '%1' specified in user_config.pri", $$fromfile(user_config.pri, CONFIG)))
}

# Bluetooth
contains (DEFINES, QGC_DISABLE_BLUETOOTH) {
    message("Skipping support for Bluetooth (manual override from command line)")
    DEFINES -= QGC_ENABLE_BLUETOOTH
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_DISABLE_BLUETOOTH) {
    message("Skipping support for Bluetooth (manual override from user_config.pri)")
    DEFINES -= QGC_ENABLE_BLUETOOTH
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_ENABLE_BLUETOOTH) {
    message("Including support for Bluetooth (manual override from user_config.pri)")
    DEFINES += QGC_ENABLE_BLUETOOTH
}

LinuxBuild {
    CONFIG += link_pkgconfig
}

# Qt configuration

CONFIG += qt \
    thread \
    c++11 \

contains(DEFINES, ENABLE_VERBOSE_OUTPUT) {
    message("Enable verbose compiler output (manual override from command line)")
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, ENABLE_VERBOSE_OUTPUT) {
    message("Enable verbose compiler output (manual override from user_config.pri)")
} else {
CONFIG += \
    silent
}

QT += \
    concurrent \
    gui \
    location \
    network \
    opengl \
    positioning \
    qml \
    quick \
    quickwidgets \
    sql \
    svg \
    widgets \
    xml \

!MobileBuild {
QT += \
    printsupport \
    serialport \
}

contains(DEFINES, QGC_ENABLE_BLUETOOTH) {
QT += \
    bluetooth \
}

#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib
ReleaseBuild {
    # We don't need the testlib console in release mode
    QT.testlib.CONFIG -= console
}
#
# OS Specific settings
#

MacBuild {
    QMAKE_INFO_PLIST    = Custom-Info.plist
    ICON                = $${BASEDIR}/resources/icons/macx.icns
    OTHER_FILES        += Custom-Info.plist
equals(QT_MAJOR_VERSION, 5) | greaterThan(QT_MINOR_VERSION, 5) {
    LIBS               += -framework ApplicationServices
}
}

iOSBuild {
    BUNDLE.files        = $$files($$PWD/ios/AppIcon*.png) $$PWD/ios/QGCLaunchScreen.xib
    QMAKE_BUNDLE_DATA  += BUNDLE
    LIBS               += -framework AVFoundation
    #-- Info.plist (need an "official" one for the App Store)
    ForAppStore {
        message(App Store Build)
        QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOSForAppStore-Info.plist
        OTHER_FILES      += $${BASEDIR}/ios/iOSForAppStore-Info.plist
    } else {
        QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOS-Info.plist
        OTHER_FILES      += $${BASEDIR}/ios/iOS-Info.plist
    }
    #-- TODO: Add iTunesArtwork
}

LinuxBuild {
    CONFIG += qesp_linux_udev
}

RC_ICONS = resources/icons/qgroundcontrol.ico
QMAKE_TARGET_COMPANY = "qgroundcontrol.org"
QMAKE_TARGET_DESCRIPTION = "Open source ground control app provided by QGroundControl dev team"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2016 QGroundControl Development Team. All rights reserved."
QMAKE_TARGET_PRODUCT = "qgroundcontrol"

#
# Build-specific settings
#

DebugBuild {
!iOSBuild {
    CONFIG += console
}
}

!MobileBuild {
# qextserialport should not be used by general QGroundControl code. Use QSerialPort instead. This is only
# here to support special case Firmware Upgrade code.
include(libs/qextserialport/src/qextserialport.pri)
}

#
# Our QtLocation "plugin"
#

include(src/QtLocationPlugin/QGCLocationPlugin.pri)

#
# External library configuration
#

include(QGCExternalLibs.pri)

#
# Main QGroundControl portion of project file
#

RESOURCES += \
    qgroundcontrol.qrc \
    qgcresources.qrc

DEPENDPATH += \
    . \
    plugins

INCLUDEPATH += .

INCLUDEPATH += \
    include/ui \
    src \
    src/audio \
    src/AutoPilotPlugins \
    src/comm \
    src/FlightDisplay \
    src/FlightMap \
    src/FlightMap/Widgets \
    src/input \
    src/Joystick \
    src/FollowMe \
    src/lib/qmapcontrol \
    src/MissionEditor \
    src/MissionManager \
    src/QmlControls \
    src/uas \
    src/ui \
    src/ui/linechart \
    src/ui/map \
    src/ui/mapdisplay \
    src/ui/mission \
    src/ui/px4_configuration \
    src/ui/toolbar \
    src/ui/uas \
    src/VehicleSetup \
    src/ViewWidgets \
    src/QtLocationPlugin \
    src/QtLocationPlugin/QMLControl \

FORMS += \
    src/ui/MainWindow.ui \
    src/QGCQmlWidgetHolder.ui \

!MobileBuild {
FORMS += \
    src/ui/uas/QGCUnconnectedInfoWidget.ui \
    src/ui/uas/UASMessageView.ui \
    src/ui/Linechart.ui \
    src/ui/MultiVehicleDockWidget.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/QGCDataPlot2D.ui \
    src/ui/QGCHilConfiguration.ui \
    src/ui/QGCHilFlightGearConfiguration.ui \
    src/ui/QGCHilJSBSimConfiguration.ui \
    src/ui/QGCHilXPlaneConfiguration.ui \
    src/ui/QGCMapRCToParamDialog.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/QGCMAVLinkLogPlayer.ui \
    src/ui/QGCTabbedInfoView.ui \
    src/ui/QGCUASFileView.ui \
    src/ui/QGCUASFileViewMulti.ui \
    src/ui/SettingsDialog.ui \
    src/ui/uas/UASQuickView.ui \
    src/ui/uas/UASQuickViewItemSelect.ui \
    src/ui/UASInfo.ui \
}

HEADERS += \
    src/audio/QGCAudioWorker.h \
    src/CmdLineOptParser.h \
    src/comm/LinkConfiguration.h \
    src/comm/LinkInterface.h \
    src/comm/LinkManager.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/ProtocolInterface.h \
    src/comm/QGCMAVLink.h \
    src/comm/TCPLink.h \
    src/comm/UDPLink.h \
    src/FlightDisplay/FlightDisplayViewController.h \
    src/FlightMap/FlightMapSettings.h \
    src/FlightMap/Widgets/ValuesWidgetController.h \
    src/GAudioOutput.h \
    src/HomePositionManager.h \
    src/Joystick/Joystick.h \
    src/Joystick/JoystickManager.h \
    src/FollowMe/FollowMe.h \
    src/JsonHelper.h \
    src/LogCompressor.h \
    src/MG.h \
    src/MissionManager/MissionCommandList.h \
    src/MissionManager/MissionCommands.h \
    src/MissionManager/MissionController.h \
    src/MissionManager/MissionItem.h \
    src/MissionManager/MissionManager.h \
    src/MissionManager/ComplexMissionItem.h \
    src/MissionManager/SimpleMissionItem.h \
    src/MissionManager/VisualMissionItem.h \
    src/QGC.h \
    src/QGCApplication.h \
    src/QGCComboBox.h \
    src/QGCConfig.h \
    src/QGCDockWidget.h \
    src/QGCFileDownload.h \
    src/QGCGeo.h \
    src/QGCLoggingCategory.h \
    src/QGCMapPalette.h \
    src/QGCMobileFileDialogController.h \
    src/QGCPalette.h \
    src/QGCQmlWidgetHolder.h \
    src/QGCQuickWidget.h \
    src/QGCTemporaryFile.h \
    src/QGCToolbox.h \
    src/QmlControls/AppMessages.h \
    src/QmlControls/CoordinateVector.h \
    src/QmlControls/MavlinkQmlSingleton.h \
    src/QmlControls/ParameterEditorController.h \
    src/QmlControls/RCChannelMonitorController.h \
    src/QmlControls/ScreenToolsController.h \
    src/QmlControls/QGCQGeoCoordinate.h \
    src/QmlControls/QGroundControlQmlGlobal.h \
    src/QmlControls/QmlObjectListModel.h \
    src/uas/UAS.h \
    src/uas/UASInterface.h \
    src/uas/UASMessageHandler.h \
    src/ui/toolbar/MainToolBarController.h \
    src/AutoPilotPlugins/PX4/PX4AirframeLoader.h \
    src/AutoPilotPlugins/APM/APMAirframeLoader.h \
    src/QmlControls/QGCImageProvider.h \
    src/AutoPilotPlugins/APM/APMRemoteParamsDownloader.h \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.h \

DebugBuild {
HEADERS += \
    src/comm/MockLink.h \
    src/comm/MockLinkFileServer.h \
    src/comm/MockLinkMissionItemHandler.h \
}

WindowsBuild {
    PRECOMPILED_HEADER += src/stable_headers.h
    HEADERS += src/stable_headers.h
    CONFIG -= silent
    OTHER_FILES += .appveyor.yml
}

contains(DEFINES, QGC_ENABLE_BLUETOOTH) {
    HEADERS += \
    src/comm/BluetoothLink.h \
}

!iOSBuild {
HEADERS += \
    src/comm/QGCSerialPortInfo.h \
    src/comm/SerialLink.h \
}

!MobileBuild {
HEADERS += \
    src/comm/LogReplayLink.h \
    src/comm/QGCFlightGearLink.h \
    src/comm/QGCHilLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
    src/QGCFileDialog.h \
    src/QGCMessageBox.h \
    src/uas/FileManager.h \
    src/ui/HILDockWidget.h \
    src/ui/linechart/ChartPlot.h \
    src/ui/linechart/IncrementalPlot.h \
    src/ui/linechart/LinechartPlot.h \
    src/ui/linechart/Linecharts.h \
    src/ui/linechart/LinechartWidget.h \
    src/ui/linechart/Scrollbar.h \
    src/ui/linechart/ScrollZoomer.h \
    src/ui/MainWindow.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/ui/MultiVehicleDockWidget.h \
    src/ui/QGCMAVLinkLogPlayer.h \
    src/ui/QGCMapRCToParamDialog.h \
    src/ui/SettingsDialog.h \
    src/ui/uas/UASMessageView.h \
    src/ui/uas/QGCUnconnectedInfoWidget.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/QGCHilConfiguration.h \
    src/ui/QGCHilFlightGearConfiguration.h \
    src/ui/QGCHilJSBSimConfiguration.h \
    src/ui/QGCHilXPlaneConfiguration.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/QGCTabbedInfoView.h \
    src/ui/QGCUASFileView.h \
    src/ui/QGCUASFileViewMulti.h \
    src/ui/uas/UASInfoWidget.h \
    src/ui/uas/UASQuickView.h \
    src/ui/uas/UASQuickViewGaugeItem.h \
    src/ui/uas/UASQuickViewItem.h \
    src/ui/uas/UASQuickViewItemSelect.h \
    src/ui/uas/UASQuickViewTextItem.h \
    src/VehicleSetup/JoystickConfigController.h \
    src/ViewWidgets/CustomCommandWidget.h \
    src/ViewWidgets/CustomCommandWidgetController.h \
    src/ViewWidgets/LogDownload.h \
    src/ViewWidgets/LogDownloadController.h \
    src/ViewWidgets/ViewWidgetController.h \
}

iOSBuild {
    OBJECTIVE_SOURCES += \
        src/audio/QGCAudioWorker_iOS.mm \
        src/MobileScreenMgr.mm \
}
AndroidBuild {
    SOURCES += src/MobileScreenMgr.cc \
}


SOURCES += \
    src/audio/QGCAudioWorker.cpp \
    src/CmdLineOptParser.cc \
    src/comm/LinkConfiguration.cc \
    src/comm/LinkManager.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/TCPLink.cc \
    src/comm/UDPLink.cc \
    src/FlightDisplay/FlightDisplayViewController.cc \
    src/FlightMap/FlightMapSettings.cc \
    src/FlightMap/Widgets/ValuesWidgetController.cc \
    src/GAudioOutput.cc \
    src/HomePositionManager.cc \
    src/Joystick/Joystick.cc \
    src/Joystick/JoystickManager.cc \
    src/JsonHelper.cc \
    src/FollowMe/FollowMe.cc \
    src/LogCompressor.cc \
    src/main.cc \
    src/MissionManager/MissionCommandList.cc \
    src/MissionManager/MissionCommands.cc \
    src/MissionManager/MissionController.cc \
    src/MissionManager/MissionItem.cc \
    src/MissionManager/MissionManager.cc \
    src/MissionManager/ComplexMissionItem.cc \
    src/MissionManager/SimpleMissionItem.cc \
    src/MissionManager/VisualMissionItem.cc \
    src/QGC.cc \
    src/QGCApplication.cc \
    src/QGCComboBox.cc \
    src/QGCDockWidget.cc \
    src/QGCFileDownload.cc \
    src/QGCLoggingCategory.cc \
    src/QGCMapPalette.cc \
    src/QGCMobileFileDialogController.cc \
    src/QGCPalette.cc \
    src/QGCQuickWidget.cc \
    src/QGCQmlWidgetHolder.cpp \
    src/QGCTemporaryFile.cc \
    src/QGCToolbox.cc \
    src/QGCGeo.cc \
    src/QmlControls/AppMessages.cc \
    src/QmlControls/CoordinateVector.cc \
    src/QmlControls/ParameterEditorController.cc \
    src/QmlControls/RCChannelMonitorController.cc \
    src/QmlControls/ScreenToolsController.cc \
    src/QmlControls/QGCQGeoCoordinate.cc \
    src/QmlControls/QGroundControlQmlGlobal.cc \
    src/QmlControls/QmlObjectListModel.cc \
    src/uas/UAS.cc \
    src/uas/UASMessageHandler.cc \
    src/ui/toolbar/MainToolBarController.cc \
    src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc \
    src/AutoPilotPlugins/APM/APMAirframeLoader.cc \
    src/QmlControls/QGCImageProvider.cc \
    src/AutoPilotPlugins/APM/APMRemoteParamsDownloader.cc \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.cc \

DebugBuild {
SOURCES += \
    src/comm/MockLink.cc \
    src/comm/MockLinkFileServer.cc \
    src/comm/MockLinkMissionItemHandler.cc \
}

!iOSBuild {
SOURCES += \
    src/comm/QGCSerialPortInfo.cc \
    src/comm/SerialLink.cc \
}

contains(DEFINES, QGC_ENABLE_BLUETOOTH) {
    SOURCES += \
    src/comm/BluetoothLink.cc \
}

!MobileBuild {
SOURCES += \
    src/ui/uas/UASMessageView.cc \
    src/uas/FileManager.cc \
    src/ui/uas/QGCUnconnectedInfoWidget.cc \
    src/ui/SettingsDialog.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/ui/QGCMapRCToParamDialog.cpp \
    src/comm/LogReplayLink.cc \
    src/QGCFileDialog.cc \
    src/ui/QGCMAVLinkLogPlayer.cc \
    src/comm/QGCFlightGearLink.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
    src/ui/HILDockWidget.cc \
    src/ui/linechart/ChartPlot.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/linechart/LinechartPlot.cc \
    src/ui/linechart/Linecharts.cc \
    src/ui/linechart/LinechartWidget.cc \
    src/ui/linechart/Scrollbar.cc \
    src/ui/linechart/ScrollZoomer.cc \
    src/ui/MainWindow.cc \
    src/ui/MultiVehicleDockWidget.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/QGCHilConfiguration.cc \
    src/ui/QGCHilFlightGearConfiguration.cc \
    src/ui/QGCHilJSBSimConfiguration.cc \
    src/ui/QGCHilXPlaneConfiguration.cc \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/QGCTabbedInfoView.cpp \
    src/ui/QGCUASFileView.cc \
    src/ui/QGCUASFileViewMulti.cc \
    src/ui/uas/UASInfoWidget.cc \
    src/ui/uas/UASQuickView.cc \
    src/ui/uas/UASQuickViewGaugeItem.cc \
    src/ui/uas/UASQuickViewItem.cc \
    src/ui/uas/UASQuickViewItemSelect.cc \
    src/ui/uas/UASQuickViewTextItem.cc \
    src/VehicleSetup/JoystickConfigController.cc \
    src/ViewWidgets/CustomCommandWidget.cc \
    src/ViewWidgets/CustomCommandWidgetController.cc \
    src/ViewWidgets/LogDownload.cc \
    src/ViewWidgets/LogDownloadController.cc \
    src/ViewWidgets/ViewWidgetController.cc
}

#
# Unit Test specific configuration goes here
#

DebugBuild {

HEADERS += src/QmlControls/QmlTestWidget.h
SOURCES += src/QmlControls/QmlTestWidget.cc

!MobileBuild {

INCLUDEPATH += \
    src/qgcunittest

HEADERS += \
    src/FactSystem/FactSystemTestBase.h \
    src/FactSystem/FactSystemTestGeneric.h \
    src/FactSystem/FactSystemTestPX4.h \
    src/MissionManager/ComplexMissionItemTest.h \
    src/MissionManager/MissionControllerTest.h \
    src/MissionManager/MissionControllerManagerTest.h \
    src/MissionManager/MissionItemTest.h \
    src/MissionManager/MissionManagerTest.h \
    src/MissionManager/SimpleMissionItemTest.h \
    src/qgcunittest/GeoTest.h \
    src/qgcunittest/FileDialogTest.h \
    src/qgcunittest/FileManagerTest.h \
    src/qgcunittest/FlightGearTest.h \
    src/qgcunittest/LinkManagerTest.h \
    src/qgcunittest/MainWindowTest.h \
    src/qgcunittest/MavlinkLogTest.h \
    src/qgcunittest/MessageBoxTest.h \
    src/qgcunittest/MultiSignalSpy.h \
    src/qgcunittest/RadioConfigTest.h \
    src/qgcunittest/TCPLinkTest.h \
    src/qgcunittest/TCPLoopBackServer.h \
    src/qgcunittest/UnitTest.h \
    src/VehicleSetup/SetupViewTest.h \

SOURCES += \
    src/FactSystem/FactSystemTestBase.cc \
    src/FactSystem/FactSystemTestGeneric.cc \
    src/FactSystem/FactSystemTestPX4.cc \
    src/MissionManager/ComplexMissionItemTest.cc \
    src/MissionManager/MissionControllerTest.cc \
    src/MissionManager/MissionControllerManagerTest.cc \
    src/MissionManager/MissionItemTest.cc \
    src/MissionManager/MissionManagerTest.cc \
    src/MissionManager/SimpleMissionItemTest.cc \
    src/qgcunittest/GeoTest.cc \
    src/qgcunittest/FileDialogTest.cc \
    src/qgcunittest/FileManagerTest.cc \
    src/qgcunittest/FlightGearTest.cc \
    src/qgcunittest/LinkManagerTest.cc \
    src/qgcunittest/MainWindowTest.cc \
    src/qgcunittest/MavlinkLogTest.cc \
    src/qgcunittest/MessageBoxTest.cc \
    src/qgcunittest/MultiSignalSpy.cc \
    src/qgcunittest/RadioConfigTest.cc \
    src/qgcunittest/TCPLinkTest.cc \
    src/qgcunittest/TCPLoopBackServer.cc \
    src/qgcunittest/UnitTest.cc \
    src/qgcunittest/UnitTestList.cc \
    src/VehicleSetup/SetupViewTest.cc \
} # !MobileBuild
} # DebugBuild

#
# Firmware Plugin Support
#

INCLUDEPATH += \
    src/AutoPilotPlugins/APM \
    src/AutoPilotPlugins/Common \
    src/AutoPilotPlugins/PX4 \
    src/FirmwarePlugin \
    src/FirmwarePlugin/APM \
    src/FirmwarePlugin/PX4 \
    src/Vehicle \
    src/VehicleSetup \

HEADERS+= \
    src/AutoPilotPlugins/AutoPilotPlugin.h \
    src/AutoPilotPlugins/AutoPilotPluginManager.h \
    src/AutoPilotPlugins/APM/APMAutoPilotPlugin.h \
    src/AutoPilotPlugins/APM/APMAirframeComponent.h \
    src/AutoPilotPlugins/APM/APMAirframeComponentController.h \
    src/AutoPilotPlugins/APM/APMAirframeComponentAirframes.h \
    src/AutoPilotPlugins/APM/APMCameraComponent.h \
    src/AutoPilotPlugins/APM/APMCompassCal.h \
    src/AutoPilotPlugins/APM/APMFlightModesComponent.h \
    src/AutoPilotPlugins/APM/APMFlightModesComponentController.h \
    src/AutoPilotPlugins/APM/APMPowerComponent.h \
    src/AutoPilotPlugins/APM/APMRadioComponent.h \
    src/AutoPilotPlugins/APM/APMSafetyComponent.h \
    src/AutoPilotPlugins/APM/APMSensorsComponent.h \
    src/AutoPilotPlugins/APM/APMSensorsComponentController.h \
    src/AutoPilotPlugins/APM/APMTuningComponent.h \
    src/AutoPilotPlugins/Common/RadioComponentController.h \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.h \
    src/AutoPilotPlugins/Common/ESP8266Component.h \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.h \
    src/AutoPilotPlugins/PX4/AirframeComponent.h \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.h \
    src/AutoPilotPlugins/PX4/AirframeComponentController.h \
    src/AutoPilotPlugins/PX4/FlightModesComponent.h \
    src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.h \
    src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.h \
    src/AutoPilotPlugins/PX4/PowerComponent.h \
    src/AutoPilotPlugins/PX4/PowerComponentController.h \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h \
    src/AutoPilotPlugins/PX4/PX4RadioComponent.h \
    src/AutoPilotPlugins/PX4/SafetyComponent.h \
    src/AutoPilotPlugins/PX4/SensorsComponent.h \
    src/AutoPilotPlugins/PX4/SensorsComponentController.h \
    src/AutoPilotPlugins/PX4/PX4TuningComponent.h \
    src/FirmwarePlugin/FirmwarePluginManager.h \
    src/FirmwarePlugin/FirmwarePlugin.h \
    src/FirmwarePlugin/APM/APMFirmwarePlugin.h \
    src/FirmwarePlugin/APM/APMParameterMetaData.h \
    src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.h \
    src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.h \
    src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.h \
    src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h \
    src/FirmwarePlugin/PX4/PX4ParameterMetaData.h \
    src/Vehicle/MultiVehicleManager.h \
    src/Vehicle/Vehicle.h \
    src/VehicleSetup/VehicleComponent.h \

!MobileBuild {
HEADERS += \
    src/VehicleSetup/FirmwareUpgradeController.h \
    src/VehicleSetup/Bootloader.h \
    src/VehicleSetup/PX4FirmwareUpgradeThread.h \
    src/VehicleSetup/FirmwareImage.h \

}

SOURCES += \
    src/AutoPilotPlugins/AutoPilotPlugin.cc \
    src/AutoPilotPlugins/AutoPilotPluginManager.cc \
    src/AutoPilotPlugins/APM/APMAutoPilotPlugin.cc \
    src/AutoPilotPlugins/APM/APMAirframeComponent.cc \
    src/AutoPilotPlugins/APM/APMAirframeComponentController.cc \
    src/AutoPilotPlugins/APM/APMCameraComponent.cc \
    src/AutoPilotPlugins/APM/APMCompassCal.cc \
    src/AutoPilotPlugins/APM/APMFlightModesComponent.cc \
    src/AutoPilotPlugins/APM/APMFlightModesComponentController.cc \
    src/AutoPilotPlugins/APM/APMPowerComponent.cc \
    src/AutoPilotPlugins/APM/APMRadioComponent.cc \
    src/AutoPilotPlugins/APM/APMSafetyComponent.cc \
    src/AutoPilotPlugins/APM/APMSensorsComponent.cc \
    src/AutoPilotPlugins/APM/APMSensorsComponentController.cc \
    src/AutoPilotPlugins/APM/APMTuningComponent.cc \
    src/AutoPilotPlugins/Common/RadioComponentController.cc \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.cc \
    src/AutoPilotPlugins/Common/ESP8266Component.cc \
    src/AutoPilotPlugins/APM/APMAirframeComponentAirframes.cc \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc \
    src/AutoPilotPlugins/PX4/AirframeComponent.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentController.cc \
    src/AutoPilotPlugins/PX4/FlightModesComponent.cc \
    src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.cc \
    src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.cc \
    src/AutoPilotPlugins/PX4/PowerComponent.cc \
    src/AutoPilotPlugins/PX4/PowerComponentController.cc \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc \
    src/AutoPilotPlugins/PX4/PX4RadioComponent.cc \
    src/AutoPilotPlugins/PX4/SafetyComponent.cc \
    src/AutoPilotPlugins/PX4/SensorsComponent.cc \
    src/AutoPilotPlugins/PX4/SensorsComponentController.cc \
    src/AutoPilotPlugins/PX4/PX4TuningComponent.cc \
    src/FirmwarePlugin/APM/APMFirmwarePlugin.cc \
    src/FirmwarePlugin/APM/APMParameterMetaData.cc \
    src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.cc \
    src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc \
    src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc \
    src/FirmwarePlugin/FirmwarePlugin.cc \
    src/FirmwarePlugin/FirmwarePluginManager.cc \
    src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc \
    src/FirmwarePlugin/PX4/PX4ParameterMetaData.cc \
    src/Vehicle/MultiVehicleManager.cc \
    src/Vehicle/Vehicle.cc \
    src/VehicleSetup/VehicleComponent.cc \

!MobileBuild {
SOURCES += \
    src/VehicleSetup/FirmwareUpgradeController.cc \
    src/VehicleSetup/Bootloader.cc \
    src/VehicleSetup/PX4FirmwareUpgradeThread.cc \
    src/VehicleSetup/FirmwareImage.cc \

}

# Fact System code

INCLUDEPATH += \
    src/FactSystem \
    src/FactSystem/FactControls \

HEADERS += \
    src/FactSystem/Fact.h \
    src/FactSystem/FactGroup.h \
    src/FactSystem/FactControls/FactPanelController.h \
    src/FactSystem/FactMetaData.h \
    src/FactSystem/FactSystem.h \
    src/FactSystem/FactValidator.h \
    src/FactSystem/ParameterLoader.h \
    src/FactSystem/SettingsFact.h \

SOURCES += \
    src/FactSystem/Fact.cc \
    src/FactSystem/FactGroup.cc \
    src/FactSystem/FactControls/FactPanelController.cc \
    src/FactSystem/FactMetaData.cc \
    src/FactSystem/FactSystem.cc \
    src/FactSystem/FactValidator.cc \
    src/FactSystem/ParameterLoader.cc \
    src/FactSystem/SettingsFact.cc \

#-------------------------------------------------------------------------------------
# Video Streaming

INCLUDEPATH += \
    src/VideoStreaming

HEADERS += \
    src/VideoStreaming/VideoItem.h \
    src/VideoStreaming/VideoReceiver.h \
    src/VideoStreaming/VideoStreaming.h \
    src/VideoStreaming/VideoSurface.h \
    src/VideoStreaming/VideoSurface_p.h \

SOURCES += \
    src/VideoStreaming/VideoItem.cc \
    src/VideoStreaming/VideoReceiver.cc \
    src/VideoStreaming/VideoStreaming.cc \
    src/VideoStreaming/VideoSurface.cc \

contains (DEFINES, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from command line)")
    DEFINES -= DISABLE_VIDEOSTREAMING
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from user_config.pri)")
} else {
    include(src/VideoStreaming/VideoStreaming.pri)
}

#-------------------------------------------------------------------------------------
# Android

AndroidBuild {
    include($$PWD/libs/qtandroidserialport/src/qtandroidserialport.pri)
    message("Adding Serial Java Classes")
    QT += androidextras
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    OTHER_FILES += \
        $$PWD/android/AndroidManifest.xml \
        $$PWD/android/res/xml/device_filter.xml \
        $$PWD/android/src/com/hoho/android/usbserial/driver/CdcAcmSerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/CommonUsbSerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/Cp2102SerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/ProlificSerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/UsbId.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
        $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
        $$PWD/android/src/org/qgroundcontrol/qgchelper/UsbDeviceJNI.java \
        $$PWD/android/src/org/qgroundcontrol/qgchelper/UsbIoManager.java

    DISTFILES += \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradlew \
        android/res/values/libs.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew.bat
}

#-------------------------------------------------------------------------------------
#
# Post link configuration
#

include(QGCSetup.pri)

#
# Installer targets
#

include(QGCInstaller.pri)
