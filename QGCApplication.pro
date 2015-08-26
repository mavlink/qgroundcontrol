# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2014 QGroundControl Developers
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

include(QGCCommon.pri)

TARGET = qgroundcontrol
TEMPLATE =  app

# Load additional config flags from user_config.pri
exists(user_config.pri):infile(user_config.pri, CONFIG) {
    CONFIG += $$fromfile(user_config.pri, CONFIG)
    message($$sprintf("Using user-supplied additional config: '%1' specified in user_config.pri", $$fromfile(user_config.pri, CONFIG)))
}

LinuxBuild {
    CONFIG += link_pkgconfig
}

# QGC QtLocation plugin

LIBS += -L$${LOCATION_PLUGIN_DESTDIR}
LIBS += -l$${LOCATION_PLUGIN_NAME}

WindowsBuild {
    PRE_TARGETDEPS += $${LOCATION_PLUGIN_DESTDIR}/$${LOCATION_PLUGIN_NAME}.lib
} else {
    PRE_TARGETDEPS += $${LOCATION_PLUGIN_DESTDIR}/lib$${LOCATION_PLUGIN_NAME}.a
}

# Qt configuration

CONFIG += qt \
    thread

QT += \
    network \
    concurrent \
    gui \
    location \
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

contains(DEFINES, QGC_NOTIFY_TUNES_ENABLED) {
    QT += multimedia
}

#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib

#
# OS Specific settings
#

MacBuild {
    QMAKE_INFO_PLIST = Custom-Info.plist
    ICON = $${BASEDIR}/resources/icons/macx.icns
    OTHER_FILES += Custom-Info.plist
}

iOSBuild {
    QMAKE_INFO_PLIST = $${BASEDIR}/ios/iOS-Info.plist
    ICON = $${BASEDIR}/resources/icons/macx.icns
    OTHER_FILES += $${BASEDIR}/iOS-Info.plist
}

LinuxBuild {
    CONFIG += qesp_linux_udev
}

WindowsBuild {
    RC_FILE = $${BASEDIR}/qgroundcontrol.rc
}

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
# External library configuration
#

include(QGCExternalLibs.pri)

#
# Main QGroundControl portion of project file
#

RESOURCES += qgroundcontrol.qrc

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
    src/input \
    src/lib/qmapcontrol \
    src/QmlControls \
    src/uas \
    src/ui \
    src/ui/flightdisplay \
    src/ui/linechart \
    src/ui/map \
    src/ui/mapdisplay \
    src/ui/mavlink \
    src/ui/mission \
    src/ui/px4_configuration \
    src/ui/toolbar \
    src/ui/uas \
    src/VehicleSetup \
    src/ViewWidgets \

FORMS += \
    src/QGCQmlWidgetHolder.ui \
    src/ui/HDDisplay.ui \
    src/ui/Linechart.ui \
    src/ui/LogReplayLinkConfigurationWidget.ui \
    src/ui/MainWindow.ui \
    src/ui/map/QGCMapTool.ui \
    src/ui/map/QGCMapToolBar.ui \
    src/ui/mavlink/QGCMAVLinkMessageSender.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/mission/QGCMissionConditionDelay.ui \
    src/ui/mission/QGCMissionDoFinishSearch.ui \
    src/ui/mission/QGCMissionDoJump.ui \
    src/ui/mission/QGCMissionDoStartSearch.ui \
    src/ui/mission/QGCMissionNavLand.ui \
    src/ui/mission/QGCMissionNavLoiterTime.ui \
    src/ui/mission/QGCMissionNavLoiterTurns.ui \
    src/ui/mission/QGCMissionNavLoiterUnlim.ui \
    src/ui/mission/QGCMissionNavReturnToLaunch.ui \
    src/ui/mission/QGCMissionNavSweep.ui \
    src/ui/mission/QGCMissionNavTakeoff.ui \
    src/ui/mission/QGCMissionNavWaypoint.ui \
    src/ui/mission/QGCMissionOther.ui \
    src/ui/QGCCommConfiguration.ui \
    src/ui/QGCDataPlot2D.ui \
    src/ui/QGCLinkConfiguration.ui \
    src/ui/QGCMapRCToParamDialog.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/QGCMAVLinkLogPlayer.ui \
    src/ui/QGCPluginHost.ui \
    src/ui/QGCTabbedInfoView.ui \
    src/ui/QGCTCPLinkConfiguration.ui \
    src/ui/QGCUASFileView.ui \
    src/ui/QGCUASFileViewMulti.ui \
    src/ui/QGCUDPLinkConfiguration.ui \
    src/ui/QGCWaypointListMulti.ui \
    src/ui/SettingsDialog.ui \
    src/ui/uas/QGCUnconnectedInfoWidget.ui \
    src/ui/uas/UASMessageView.ui \
    src/ui/uas/UASQuickView.ui \
    src/ui/uas/UASQuickViewItemSelect.ui \
    src/ui/UASControl.ui \
    src/ui/UASInfo.ui \
    src/ui/UASList.ui \
    src/ui/UASRawStatusView.ui \
    src/ui/UASView.ui \
    src/ui/WaypointEditableView.ui \
    src/ui/WaypointList.ui \
    src/ui/WaypointViewOnlyView.ui \

!iOSBuild {
FORMS += \
    src/ui/SerialSettings.ui \
}

!MobileBuild {
FORMS += \
    src/ui/JoystickButton.ui \
    src/ui/JoystickAxis.ui \
    src/ui/JoystickWidget.ui \
    src/ui/QGCHilConfiguration.ui \
    src/ui/QGCHilFlightGearConfiguration.ui \
    src/ui/QGCHilJSBSimConfiguration.ui \
    src/ui/QGCHilXPlaneConfiguration.ui \
}

HEADERS += \
    src/audio/QGCAudioWorker.h \
    src/CmdLineOptParser.h \
    src/comm/LinkConfiguration.h \
    src/comm/LinkInterface.h \
    src/comm/LinkManager.h \
    src/comm/LogReplayLink.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/MockLink.h \
    src/comm/MockLinkFileServer.h \
    src/comm/MockLinkMissionItemHandler.h \
    src/comm/ProtocolInterface.h \
    src/comm/QGCMAVLink.h \
    src/comm/TCPLink.h \
    src/comm/UDPLink.h \
    src/GAudioOutput.h \
    src/LogCompressor.h \
    src/MG.h \
    src/QGC.h \
    src/QGCApplication.h \
    src/QGCComboBox.h \
    src/QGCConfig.h \
    src/QGCDockWidget.h \
    src/QGCFileDialog.h \
    src/QGCGeo.h \
    src/QGCLoggingCategory.h \
    src/QGCMessageBox.h \
    src/QGCPalette.h \
    src/QGCQmlWidgetHolder.h \
    src/QGCQuickWidget.h \
    src/QGCSingleton.h \
    src/QGCTemporaryFile.h \
    src/QmlControls/MavManager.h \
    src/QmlControls/ParameterEditorController.h \
    src/QmlControls/ScreenToolsController.h \
    src/SerialPortIds.h \
    src/uas/QGCMAVLinkUASFactory.h \
    src/uas/FileManager.h \
    src/uas/UAS.h \
    src/uas/UASInterface.h \
    src/uas/UASManager.h \
    src/uas/UASManagerInterface.h \
    src/uas/UASMessageHandler.h \
    src/uas/UASWaypointManager.h \
    src/ui/flightdisplay/FlightDisplay.h \
    src/ui/HDDisplay.h \
    src/ui/HSIDisplay.h \
    src/ui/HUD.h \
    src/ui/linechart/ChartPlot.h \
    src/ui/linechart/IncrementalPlot.h \
    src/ui/linechart/LinechartPlot.h \
    src/ui/linechart/Linecharts.h \
    src/ui/linechart/LinechartWidget.h \
    src/ui/linechart/Scrollbar.h \
    src/ui/linechart/ScrollZoomer.h \
    src/ui/LogReplayLinkConfigurationWidget.h \
    src/ui/MainWindow.h \
    src/ui/map/MAV2DIcon.h \
    src/ui/map/QGCMapTool.h \
    src/ui/map/QGCMapToolBar.h \
    src/ui/map/QGCMapWidget.h \
    src/ui/map/Waypoint2DIcon.h \
    src/ui/mapdisplay/QGCMapDisplay.h \
    src/ui/mavlink/QGCMAVLinkMessageSender.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/ui/mission/QGCMissionConditionDelay.h \
    src/ui/mission/QGCMissionDoFinishSearch.h \
    src/ui/mission/QGCMissionDoJump.h \
    src/ui/mission/QGCMissionDoStartSearch.h \
    src/ui/mission/QGCMissionNavLand.h \
    src/ui/mission/QGCMissionNavLoiterTime.h \
    src/ui/mission/QGCMissionNavLoiterTurns.h \
    src/ui/mission/QGCMissionNavLoiterUnlim.h \
    src/ui/mission/QGCMissionNavReturnToLaunch.h \
    src/ui/mission/QGCMissionNavSweep.h \
    src/ui/mission/QGCMissionNavTakeoff.h \
    src/ui/mission/QGCMissionNavWaypoint.h \
    src/ui/mission/QGCMissionOther.h \
    src/ui/QGCCommConfiguration.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/QGCLinkConfiguration.h \
    src/ui/QGCMainWindowAPConfigurator.h \
    src/ui/QGCMapRCToParamDialog.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/QGCMAVLinkLogPlayer.h \
    src/ui/QGCPluginHost.h \
    src/ui/QGCRGBDView.h \
    src/ui/QGCTabbedInfoView.h \
    src/ui/QGCTCPLinkConfiguration.h \
    src/ui/QGCUASFileView.h \
    src/ui/QGCUASFileViewMulti.h \
    src/ui/QGCUDPLinkConfiguration.h \
    src/ui/QGCWaypointListMulti.h \
    src/ui/SettingsDialog.h \
    src/ui/toolbar/MainToolBar.h \
    src/ui/uas/QGCUnconnectedInfoWidget.h \
    src/ui/uas/UASControlWidget.h \
    src/ui/uas/UASInfoWidget.h \
    src/ui/uas/UASListWidget.h \
    src/ui/uas/UASMessageView.h \
    src/ui/uas/UASQuickView.h \
    src/ui/uas/UASQuickViewGaugeItem.h \
    src/ui/uas/UASQuickViewItem.h \
    src/ui/uas/UASQuickViewItemSelect.h \
    src/ui/uas/UASQuickViewTextItem.h \
    src/ui/uas/UASView.h \
    src/ui/UASRawStatusView.h \
    src/ui/WaypointEditableView.h \
    src/ui/WaypointList.h \
    src/ui/WaypointViewOnlyView.h \
    src/ViewWidgets/CustomCommandWidget.h \
    src/ViewWidgets/CustomCommandWidgetController.h \
    src/ViewWidgets/ParameterEditorWidget.h \
    src/ViewWidgets/ViewWidgetController.h \
    src/Waypoint.h \
    src/AutoPilotPlugins/PX4/PX4AirframeLoader.h

!iOSBuild {
HEADERS += \
    src/comm/SerialLink.h \
    src/ui/SerialConfigurationWindow.h \
}

!MobileBuild {
HEADERS += \
    src/comm/QGCFlightGearLink.h \
    src/comm/QGCHilLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
    src/input/JoystickInput.h \
    src/ui/CameraView.h \
    src/ui/JoystickAxis.h \
    src/ui/JoystickButton.h \
    src/ui/JoystickWidget.h \
    src/ui/QGCHilConfiguration.h \
    src/ui/QGCHilFlightGearConfiguration.h \
    src/ui/QGCHilJSBSimConfiguration.h \
    src/ui/QGCHilXPlaneConfiguration.h \
}

SOURCES += \
    src/audio/QGCAudioWorker.cpp \
    src/CmdLineOptParser.cc \
    src/comm/LinkConfiguration.cc \
    src/comm/LinkManager.cc \
    src/comm/LogReplayLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/MockLink.cc \
    src/comm/MockLinkFileServer.cc \
    src/comm/MockLinkMissionItemHandler.cc \
    src/comm/TCPLink.cc \
    src/comm/UDPLink.cc \
    src/GAudioOutput.cc \
    src/LogCompressor.cc \
    src/main.cc \
    src/QGC.cc \
    src/QGCApplication.cc \
    src/QGCComboBox.cc \
    src/QGCDockWidget.cc \
    src/QGCFileDialog.cc \
    src/QGCLoggingCategory.cc \
    src/QGCPalette.cc \
    src/QGCQmlWidgetHolder.cpp \
    src/QGCQuickWidget.cc \
    src/QGCSingleton.cc \
    src/QGCTemporaryFile.cc \
    src/QmlControls/MavManager.cc \
    src/QmlControls/ParameterEditorController.cc \
    src/QmlControls/ScreenToolsController.cc \
    src/uas/QGCMAVLinkUASFactory.cc \
    src/uas/FileManager.cc \
    src/uas/UAS.cc \
    src/uas/UASManager.cc \
    src/uas/UASMessageHandler.cc \
    src/uas/UASWaypointManager.cc \
    src/ui/flightdisplay/FlightDisplay.cc \
    src/ui/HDDisplay.cc \
    src/ui/HSIDisplay.cc \
    src/ui/HUD.cc \
    src/ui/linechart/ChartPlot.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/linechart/LinechartPlot.cc \
    src/ui/linechart/Linecharts.cc \
    src/ui/linechart/LinechartWidget.cc \
    src/ui/linechart/Scrollbar.cc \
    src/ui/linechart/ScrollZoomer.cc \
    src/ui/LogReplayLinkConfigurationWidget.cc \
    src/ui/MainWindow.cc \
    src/ui/map/MAV2DIcon.cc \
    src/ui/map/QGCMapTool.cc \
    src/ui/map/QGCMapToolBar.cc \
    src/ui/map/QGCMapWidget.cc \
    src/ui/map/Waypoint2DIcon.cc \
    src/ui/mapdisplay/QGCMapDisplay.cc \
    src/ui/mavlink/QGCMAVLinkMessageSender.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/ui/mission/QGCMissionConditionDelay.cc \
    src/ui/mission/QGCMissionDoFinishSearch.cc \
    src/ui/mission/QGCMissionDoJump.cc \
    src/ui/mission/QGCMissionDoStartSearch.cc \
    src/ui/mission/QGCMissionNavLand.cc \
    src/ui/mission/QGCMissionNavLoiterTime.cc \
    src/ui/mission/QGCMissionNavLoiterTurns.cc \
    src/ui/mission/QGCMissionNavLoiterUnlim.cc \
    src/ui/mission/QGCMissionNavReturnToLaunch.cc \
    src/ui/mission/QGCMissionNavSweep.cc \
    src/ui/mission/QGCMissionNavTakeoff.cc \
    src/ui/mission/QGCMissionNavWaypoint.cc \
    src/ui/mission/QGCMissionOther.cc \
    src/ui/QGCCommConfiguration.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/QGCLinkConfiguration.cc \
    src/ui/QGCMainWindowAPConfigurator.cc \
    src/ui/QGCMapRCToParamDialog.cpp \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/QGCMAVLinkLogPlayer.cc \
    src/ui/QGCPluginHost.cc \
    src/ui/QGCRGBDView.cc \
    src/ui/QGCTabbedInfoView.cpp \
    src/ui/QGCTCPLinkConfiguration.cc \
    src/ui/QGCUASFileView.cc \
    src/ui/QGCUASFileViewMulti.cc \
    src/ui/QGCUDPLinkConfiguration.cc \
    src/ui/QGCWaypointListMulti.cc \
    src/ui/SettingsDialog.cc \
    src/ui/toolbar/MainToolBar.cc \
    src/ui/uas/QGCUnconnectedInfoWidget.cc \
    src/ui/uas/UASControlWidget.cc \
    src/ui/uas/UASInfoWidget.cc \
    src/ui/uas/UASListWidget.cc \
    src/ui/uas/UASMessageView.cc \
    src/ui/uas/UASQuickView.cc \
    src/ui/uas/UASQuickViewGaugeItem.cc \
    src/ui/uas/UASQuickViewItem.cc \
    src/ui/uas/UASQuickViewItemSelect.cc \
    src/ui/uas/UASQuickViewTextItem.cc \
    src/ui/uas/UASView.cc \
    src/ui/UASRawStatusView.cpp \
    src/ui/WaypointEditableView.cc \
    src/ui/WaypointList.cc \
    src/ui/WaypointViewOnlyView.cc \
    src/ViewWidgets/CustomCommandWidget.cc \
    src/ViewWidgets/CustomCommandWidgetController.cc \
    src/ViewWidgets/ParameterEditorWidget.cc \
    src/ViewWidgets/ViewWidgetController.cc \
    src/Waypoint.cc \
    src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc

!iOSBuild {
SOURCES += \
    src/comm/SerialLink.cc \
    src/ui/SerialConfigurationWindow.cc \
}

!MobileBuild {
SOURCES += \
    src/comm/QGCFlightGearLink.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
    src/input/JoystickInput.cc \
    src/ui/CameraView.cc \
    src/ui/JoystickAxis.cc \
    src/ui/JoystickButton.cc \
    src/ui/JoystickWidget.cc \
    src/ui/QGCHilConfiguration.cc \
    src/ui/QGCHilFlightGearConfiguration.cc \
    src/ui/QGCHilJSBSimConfiguration.cc \
    src/ui/QGCHilXPlaneConfiguration.cc \
}

#
# Unit Test specific configuration goes here
#
# We have to special case Windows debug_and_release builds because you can't have files
# which are only in the debug variant [QTBUG-40351]. So in this case we include unit tests
# even in the release variant. If you want a Windows release build with no unit tests run
# qmake with CONFIG-=debug_and_release CONFIG+=release.
#

DebugBuild|WindowsDebugAndRelease {

HEADERS += src/QmlControls/QmlTestWidget.h
SOURCES += src/QmlControls/QmlTestWidget.cc

!MobileBuild {

INCLUDEPATH += \
	src/qgcunittest

HEADERS += \
    src/qgcunittest/FlightGearTest.h \
    src/qgcunittest/MultiSignalSpy.h \
    src/qgcunittest/TCPLinkTest.h \
    src/qgcunittest/TCPLoopBackServer.h \
    src/FactSystem/FactSystemTestBase.h \
    src/FactSystem/FactSystemTestGeneric.h \
    src/FactSystem/FactSystemTestPX4.h \
    src/qgcunittest/FileDialogTest.h \
    src/qgcunittest/LinkManagerTest.h \
    src/qgcunittest/MainWindowTest.h \
    src/qgcunittest/MavlinkLogTest.h \
    src/qgcunittest/MessageBoxTest.h \
    src/qgcunittest/UnitTest.h \
    src/VehicleSetup/SetupViewTest.h \
    src/qgcunittest/FileManagerTest.h \
    src/qgcunittest/PX4RCCalibrationTest.h \

SOURCES += \
    src/qgcunittest/FlightGearTest.cc \
    src/qgcunittest/MultiSignalSpy.cc \
    src/qgcunittest/TCPLinkTest.cc \
    src/qgcunittest/TCPLoopBackServer.cc \
    src/FactSystem/FactSystemTestBase.cc \
    src/FactSystem/FactSystemTestGeneric.cc \
    src/FactSystem/FactSystemTestPX4.cc \
    src/qgcunittest/FileDialogTest.cc \
    src/qgcunittest/LinkManagerTest.cc \
    src/qgcunittest/MainWindowTest.cc \
    src/qgcunittest/MavlinkLogTest.cc \
    src/qgcunittest/MessageBoxTest.cc \
    src/qgcunittest/UnitTest.cc \
    src/VehicleSetup/SetupViewTest.cc \
    src/qgcunittest/FileManagerTest.cc \
    src/qgcunittest/PX4RCCalibrationTest.cc \

} # DebugBuild|WindowsDebugAndRelease
} # MobileBuild

#
# Firmware Plugin Support
#

INCLUDEPATH += \
    src/FirmwarePlugin \
    src/VehicleSetup \
    src/AutoPilotPlugins/PX4 \

HEADERS+= \
    src/FirmwarePlugin/FirmwarePluginManager.h \
    src/FirmwarePlugin/FirmwarePlugin.h \
    src/FirmwarePlugin/Generic/GenericFirmwarePlugin.h \
    src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h \
    src/AutoPilotPlugins/AutoPilotPlugin.h \
    src/AutoPilotPlugins/AutoPilotPluginManager.h \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.h \
    src/AutoPilotPlugins/Generic/GenericParameterFacts.h \
    src/AutoPilotPlugins/PX4/AirframeComponent.h \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.h \
    src/AutoPilotPlugins/PX4/AirframeComponentController.h \
    src/AutoPilotPlugins/PX4/FlightModesComponent.h \
    src/AutoPilotPlugins/PX4/FlightModesComponentController.h \
    src/AutoPilotPlugins/PX4/PowerComponent.h \
    src/AutoPilotPlugins/PX4/PowerComponentController.h \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h \
    src/AutoPilotPlugins/PX4/PX4Component.h \
    src/AutoPilotPlugins/PX4/PX4ParameterLoader.h \
    src/AutoPilotPlugins/PX4/RadioComponent.h \
    src/AutoPilotPlugins/PX4/RadioComponentController.h \
    src/AutoPilotPlugins/PX4/SafetyComponent.h \
    src/AutoPilotPlugins/PX4/SensorsComponent.h \
    src/AutoPilotPlugins/PX4/SensorsComponentController.h \
    src/VehicleSetup/SetupView.h \
    src/VehicleSetup/VehicleComponent.h \

!MobileBuild {
HEADERS += \
    src/VehicleSetup/FirmwareUpgradeController.h \
    src/VehicleSetup/Bootloader.h \
    src/VehicleSetup/PX4FirmwareUpgradeThread.h \
    src/VehicleSetup/FirmwareImage.h \

}

SOURCES += \
    src/FirmwarePlugin/FirmwarePluginManager.cc \
    src/FirmwarePlugin/Generic/GenericFirmwarePlugin.cc \
    src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc \
    src/AutoPilotPlugins/AutoPilotPlugin.cc \
    src/AutoPilotPlugins/AutoPilotPluginManager.cc \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc \
    src/AutoPilotPlugins/Generic/GenericParameterFacts.cc \
    src/AutoPilotPlugins/PX4/AirframeComponent.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentController.cc \
    src/AutoPilotPlugins/PX4/FlightModesComponent.cc \
    src/AutoPilotPlugins/PX4/FlightModesComponentController.cc \
    src/AutoPilotPlugins/PX4/PowerComponent.cc \
    src/AutoPilotPlugins/PX4/PowerComponentController.cc \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc \
    src/AutoPilotPlugins/PX4/PX4Component.cc \
    src/AutoPilotPlugins/PX4/PX4ParameterLoader.cc \
    src/AutoPilotPlugins/PX4/RadioComponent.cc \
    src/AutoPilotPlugins/PX4/RadioComponentController.cc \
    src/AutoPilotPlugins/PX4/SafetyComponent.cc \
    src/AutoPilotPlugins/PX4/SensorsComponent.cc \
    src/AutoPilotPlugins/PX4/SensorsComponentController.cc \
    src/VehicleSetup/SetupView.cc \
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
    src/FactSystem/FactMetaData.h \
    src/FactSystem/FactSystem.h \
    src/FactSystem/FactValidator.h \
    src/FactSystem/ParameterLoader.h \
    src/FactSystem/FactControls/FactPanelController.h \

SOURCES += \
    src/FactSystem/Fact.cc \
    src/FactSystem/FactMetaData.cc \
    src/FactSystem/FactSystem.cc \
    src/FactSystem/FactValidator.cc \
    src/FactSystem/ParameterLoader.cc \
    src/FactSystem/FactControls/FactPanelController.cc \

#-------------------------------------------------------------------------------------
# Video Streaming

INCLUDEPATH += \
    src/VideoStreaming

HEADERS += \
    src/VideoStreaming/VideoItem.h \
    src/VideoStreaming/VideoReceiver.h \
    src/VideoStreaming/VideoSurface.h \
    src/VideoStreaming/VideoSurface_p.h \

SOURCES += \
    src/VideoStreaming/VideoItem.cc \
    src/VideoStreaming/VideoReceiver.cc \
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
