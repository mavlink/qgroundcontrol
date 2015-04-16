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

# Load additional config flags from user_config.pri
exists(user_config.pri):infile(user_config.pri, CONFIG) {
    CONFIG += $$fromfile(user_config.pri, CONFIG)
    message($$sprintf("Using user-supplied additional config: '%1' specified in user_config.pri", $$fromfile(user_config.pri, CONFIG)))
}

LinuxBuild {
    CONFIG += link_pkgconfig
}

message(BASEDIR $$BASEDIR DESTDIR $$DESTDIR TARGET $$TARGET)

# QGC QtLocation plugin

LIBS += -L$${LOCATION_PLUGIN_DESTDIR}
LIBS += -l$${LOCATION_PLUGIN_NAME}

LinuxBuild|MacBuild {
    PRE_TARGETDEPS += $${LOCATION_PLUGIN_DESTDIR}/lib$${LOCATION_PLUGIN_NAME}.a
}

WindowsBuild {
    PRE_TARGETDEPS += $${LOCATION_PLUGIN_DESTDIR}/$${LOCATION_PLUGIN_NAME}.lib
}

# Qt configuration

CONFIG += qt \
    thread

QT += network \
    opengl \
    svg \
    xml \
    concurrent \
    widgets \
    gui \
    serialport \
    sql \
    printsupport \
    qml \
    quick \
    quickwidgets \
    location \
    positioning

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
    ICON = $$BASEDIR/resources/icons/macx.icns
    QT += quickwidgets
}

LinuxBuild {
    CONFIG += qesp_linux_udev
}

WindowsBuild {
	RC_FILE = $$BASEDIR/qgroundcontrol.rc
}

#
# Build-specific settings
#

DebugBuild {
    CONFIG += console
}

# qextserialport should not be used by general QGroundControl code. Use QSerialPort instead. This is only
# here to support special case Firmware Upgrade code.
include(libs/qextserialport/src/qextserialport.pri)

#
# External library configuration
#

include(QGCExternalLibs.pri)

#
# Post link configuration
#

include(QGCSetup.pri)

#
# Installer targets
#

include(QGCInstaller.pri)

#
# Main QGroundControl portion of project file
#

RESOURCES += qgroundcontrol.qrc

DEPENDPATH += \
    . \
    plugins

INCLUDEPATH += .

INCLUDEPATH += \
    src \
    src/ui \
    src/ui/linechart \
    src/ui/uas \
    src/ui/map \
    src/uas \
    src/comm \
    src/audio \
    include/ui \
    src/input \
    src/lib/qmapcontrol \
    src/ui/mavlink \
    src/ui/map3D \
    src/ui/mission \
    src/ui/designer \
    src/ui/configuration \
    src/ui/px4_configuration \
    src/ui/main \
    src/ui/toolbar \
    src/ui/flightdisplay \
    src/ui/mapdisplay \
    src/VehicleSetup \
    src/AutoPilotPlugins \
    src/QmlControls \
    src/ViewWidgets

FORMS += \
    src/ui/MainWindow.ui \
    src/ui/SerialSettings.ui \
    src/ui/UASControl.ui \
    src/ui/UASList.ui \
    src/ui/UASInfo.ui \
    src/ui/Linechart.ui \
    src/ui/UASView.ui \
    src/ui/WaypointList.ui \
    src/ui/JoystickWidget.ui \
    src/ui/DebugConsole.ui \
    src/ui/HDDisplay.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/QGCDataPlot2D.ui \
    src/ui/QMap3D.ui \
    src/ui/uas/QGCUnconnectedInfoWidget.ui \
    src/ui/designer/QGCToolWidget.ui \
    src/ui/designer/QGCParamSlider.ui \
    src/ui/designer/QGCActionButton.ui \
    src/ui/designer/QGCCommandButton.ui \
    src/ui/designer/QGCToolWidgetComboBox.ui \
    src/ui/designer/QGCTextLabel.ui \
    src/ui/designer/QGCXYPlot.ui \
    src/ui/QGCMAVLinkLogPlayer.ui \
    src/ui/QGCWaypointListMulti.ui \
    src/ui/QGCUASFileViewMulti.ui \
    src/ui/QGCTCPLinkConfiguration.ui \
    src/ui/SettingsDialog.ui \
    src/ui/map/QGCMapTool.ui \
    src/ui/map/QGCMapToolBar.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/WaypointViewOnlyView.ui \
    src/ui/WaypointEditableView.ui \
    src/ui/mavlink/QGCMAVLinkMessageSender.ui \
    src/ui/QGCPluginHost.ui \
    src/ui/mission/QGCMissionOther.ui \
    src/ui/mission/QGCMissionNavWaypoint.ui \
    src/ui/mission/QGCMissionDoJump.ui \
    src/ui/mission/QGCMissionConditionDelay.ui \
    src/ui/mission/QGCMissionNavLoiterUnlim.ui \
    src/ui/mission/QGCMissionNavLoiterTurns.ui \
    src/ui/mission/QGCMissionNavLoiterTime.ui \
    src/ui/mission/QGCMissionNavReturnToLaunch.ui \
    src/ui/mission/QGCMissionNavLand.ui \
    src/ui/mission/QGCMissionNavTakeoff.ui \
    src/ui/mission/QGCMissionNavSweep.ui \
    src/ui/mission/QGCMissionDoStartSearch.ui \
    src/ui/mission/QGCMissionDoFinishSearch.ui \
    src/ui/QGCHilConfiguration.ui \
    src/ui/QGCHilFlightGearConfiguration.ui \
    src/ui/QGCHilJSBSimConfiguration.ui \
    src/ui/QGCHilXPlaneConfiguration.ui \
    src/ui/uas/UASQuickView.ui \
    src/ui/uas/UASQuickViewItemSelect.ui \
    src/ui/QGCTabbedInfoView.ui \
    src/ui/UASRawStatusView.ui \
    src/ui/uas/UASMessageView.ui \
    src/ui/JoystickButton.ui \
    src/ui/JoystickAxis.ui \
    src/ui/configuration/terminalconsole.ui \
    src/ui/configuration/SerialSettingsDialog.ui \
    src/ui/px4_configuration/PX4RCCalibration.ui \
    src/ui/QGCUASFileView.ui \
    src/QGCQmlWidgetHolder.ui \
    src/ui/QGCMapRCToParamDialog.ui \
    src/ui/QGCLinkConfiguration.ui \
    src/ui/QGCCommConfiguration.ui \
    src/ui/QGCUDPLinkConfiguration.ui

HEADERS += \
    src/MG.h \
    src/QGCApplication.h \
    src/QGCSingleton.h \
    src/uas/UASInterface.h \
    src/uas/UAS.h \
    src/uas/UASManager.h \
    src/comm/LinkManager.h \
    src/comm/LinkInterface.h \
    src/comm/SerialLink.h \
    src/comm/ProtocolInterface.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/QGCFlightGearLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
    src/ui/SerialConfigurationWindow.h \
    src/ui/MainWindow.h \
    src/ui/uas/UASControlWidget.h \
    src/ui/uas/UASListWidget.h \
    src/ui/uas/UASInfoWidget.h \
    src/ui/HUD.h \
    src/ui/linechart/LinechartWidget.h \
    src/ui/linechart/LinechartPlot.h \
    src/ui/linechart/Scrollbar.h \
    src/ui/linechart/ScrollZoomer.h \
    src/QGCConfig.h \
    src/ui/uas/UASView.h \
    src/ui/CameraView.h \
    src/comm/MAVLinkSimulationLink.h \
    src/comm/UDPLink.h \
    src/comm/TCPLink.h \
    src/ViewWidgets/ParameterEditorWidget.h \
    src/ui/WaypointList.h \
    src/Waypoint.h \
    src/input/JoystickInput.h \
    src/ui/JoystickWidget.h \
    src/ui/DebugConsole.h \
    src/ui/HDDisplay.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/GAudioOutput.h \
    src/LogCompressor.h \
    src/ui/linechart/Linecharts.h \
    src/uas/UASWaypointManager.h \
    src/ui/HSIDisplay.h \
    src/QGC.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/linechart/IncrementalPlot.h \
    src/comm/QGCMAVLink.h \
    src/ui/QGCMainWindowAPConfigurator.h \
    src/comm/MAVLinkSwarmSimulationLink.h \
    src/ui/uas/QGCUnconnectedInfoWidget.h \
    src/ui/designer/QGCToolWidget.h \
    src/ui/designer/QGCParamSlider.h \
    src/ui/designer/QGCCommandButton.h \
    src/ui/designer/QGCToolWidgetItem.h \
    src/ui/designer/QGCToolWidgetComboBox.h \
    src/ui/designer/QGCTextLabel.h \
    src/ui/designer/QGCRadioChannelDisplay.h \
    src/ui/designer/QGCXYPlot.h \
    src/ui/designer/RCChannelWidget.h \
    src/ui/QGCMAVLinkLogPlayer.h \
    src/comm/MAVLinkSimulationWaypointPlanner.h \
    src/comm/MAVLinkSimulationMAV.h \
    src/uas/QGCMAVLinkUASFactory.h \
    src/ui/QGCWaypointListMulti.h \
    src/ui/QGCUASFileViewMulti.h \
    src/ui/QGCTCPLinkConfiguration.h \
    src/ui/SettingsDialog.h \
    src/uas/QGCUASParamManager.h \
    src/ui/map/QGCMapWidget.h \
    src/ui/map/MAV2DIcon.h \
    src/ui/map/Waypoint2DIcon.h \
    src/ui/map/QGCMapTool.h \
    src/ui/map/QGCMapToolBar.h \
    src/QGCGeo.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/WaypointViewOnlyView.h \
    src/ui/WaypointEditableView.h \
    src/ui/QGCRGBDView.h \
    src/ui/mavlink/QGCMAVLinkMessageSender.h \
    src/ui/QGCPluginHost.h \
    src/ui/mission/QGCMissionOther.h \
    src/ui/mission/QGCMissionNavWaypoint.h \
    src/ui/mission/QGCMissionDoJump.h \
    src/ui/mission/QGCMissionConditionDelay.h \
    src/ui/mission/QGCMissionNavLoiterUnlim.h \
    src/ui/mission/QGCMissionNavLoiterTurns.h \
    src/ui/mission/QGCMissionNavLoiterTime.h \
    src/ui/mission/QGCMissionNavReturnToLaunch.h \
    src/ui/mission/QGCMissionNavLand.h \
    src/ui/mission/QGCMissionNavTakeoff.h \
    src/ui/mission/QGCMissionNavSweep.h \
    src/ui/mission/QGCMissionDoStartSearch.h \
    src/ui/mission/QGCMissionDoFinishSearch.h \
    src/comm/QGCHilLink.h \
    src/ui/QGCHilConfiguration.h \
    src/ui/QGCHilFlightGearConfiguration.h \
    src/ui/QGCHilJSBSimConfiguration.h \
    src/ui/QGCHilXPlaneConfiguration.h \
    src/ui/uas/UASQuickView.h \
    src/ui/uas/UASQuickViewItem.h \
    src/ui/linechart/ChartPlot.h \
    src/ui/uas/UASQuickViewItemSelect.h \
    src/ui/uas/UASQuickViewTextItem.h \
    src/ui/uas/UASQuickViewGaugeItem.h \
    src/ui/QGCTabbedInfoView.h \
    src/ui/UASRawStatusView.h \
    src/ui/uas/UASMessageView.h \
    src/ui/JoystickButton.h \
    src/ui/JoystickAxis.h \
    src/ui/configuration/console.h \
    src/ui/configuration/SerialSettingsDialog.h \
    src/ui/configuration/terminalconsole.h \
    src/ui/configuration/ApmHighlighter.h \
    src/uas/UASParameterDataModel.h \
    src/uas/UASParameterCommsMgr.h \
    src/ui/px4_configuration/PX4RCCalibration.h \
    src/ui/px4_configuration/RCValueWidget.h \
    src/uas/UASManagerInterface.h \
    src/uas/QGCUASParamManagerInterface.h \
    src/uas/QGCUASFileManager.h \
    src/ui/QGCUASFileView.h \
    src/CmdLineOptParser.h \
    src/QGCFileDialog.h \
    src/QGCMessageBox.h \
    src/QGCComboBox.h \
    src/QGCTemporaryFile.h \
    src/audio/QGCAudioWorker.h \
    src/QGCQuickWidget.h \
    src/QGCPalette.h \
    src/QGCQmlWidgetHolder.h \
    src/ui/QGCMapRCToParamDialog.h \
    src/QGCDockWidget.h \
    src/ui/QGCLinkConfiguration.h \
    src/comm/LinkConfiguration.h \
    src/ui/QGCCommConfiguration.h \
    src/ui/QGCUDPLinkConfiguration.h \
    src/uas/UASMessageHandler.h \
    src/ui/toolbar/MainToolBar.h \
    src/QmlControls/ScreenTools.h \
    src/QmlControls/ParameterEditorController.h \
    src/QGCLoggingCategory.h \
    src/ui/flightdisplay/QGCFlightDisplay.h \
    src/ui/mapdisplay/QGCMapDisplay.h \
    src/ViewWidgets/ViewWidgetController.h \

SOURCES += \
    src/main.cc \
    src/QGCApplication.cc \
    src/QGCSingleton.cc \
    src/uas/UASManager.cc \
    src/uas/UAS.cc \
    src/comm/LinkManager.cc \
    src/comm/SerialLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/QGCFlightGearLink.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
    src/ui/SerialConfigurationWindow.cc \
    src/ui/MainWindow.cc \
    src/ui/uas/UASControlWidget.cc \
    src/ui/uas/UASListWidget.cc \
    src/ui/uas/UASInfoWidget.cc \
    src/ui/HUD.cc \
    src/ui/linechart/LinechartWidget.cc \
    src/ui/linechart/LinechartPlot.cc \
    src/ui/linechart/Scrollbar.cc \
    src/ui/linechart/ScrollZoomer.cc \
    src/ui/uas/UASView.cc \
    src/ui/CameraView.cc \
    src/comm/MAVLinkSimulationLink.cc \
    src/comm/UDPLink.cc \
    src/comm/TCPLink.cc \
    src/ViewWidgets/ParameterEditorWidget.cc \
    src/ui/WaypointList.cc \
    src/Waypoint.cc \
    src/input/JoystickInput.cc \
    src/ui/JoystickWidget.cc \
    src/ui/DebugConsole.cc \
    src/ui/HDDisplay.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/GAudioOutput.cc \
    src/LogCompressor.cc \
    src/ui/linechart/Linecharts.cc \
    src/uas/UASWaypointManager.cc \
    src/ui/HSIDisplay.cc \
    src/QGC.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/QGCMainWindowAPConfigurator.cc \
    src/comm/MAVLinkSwarmSimulationLink.cc \
    src/ui/uas/QGCUnconnectedInfoWidget.cc \
    src/ui/designer/QGCToolWidget.cc \
    src/ui/designer/QGCParamSlider.cc \
    src/ui/designer/QGCCommandButton.cc \
    src/ui/designer/QGCToolWidgetItem.cc \
    src/ui/designer/QGCToolWidgetComboBox.cc \
    src/ui/designer/QGCTextLabel.cc \
    src/ui/designer/QGCRadioChannelDisplay.cpp \
    src/ui/designer/QGCXYPlot.cc \
    src/ui/designer/RCChannelWidget.cc \
    src/ui/QGCMAVLinkLogPlayer.cc \
    src/comm/MAVLinkSimulationWaypointPlanner.cc \
    src/comm/MAVLinkSimulationMAV.cc \
    src/uas/QGCMAVLinkUASFactory.cc \
    src/ui/QGCWaypointListMulti.cc \
    src/ui/QGCUASFileViewMulti.cc \
    src/ui/QGCTCPLinkConfiguration.cc \
    src/ui/SettingsDialog.cc \
    src/uas/QGCUASParamManager.cc \
    src/ui/map/QGCMapWidget.cc \
    src/ui/map/MAV2DIcon.cc \
    src/ui/map/Waypoint2DIcon.cc \
    src/ui/map/QGCMapTool.cc \
    src/ui/map/QGCMapToolBar.cc \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/WaypointViewOnlyView.cc \
    src/ui/WaypointEditableView.cc \
    src/ui/QGCRGBDView.cc \
    src/ui/mavlink/QGCMAVLinkMessageSender.cc \
    src/ui/QGCPluginHost.cc \
    src/ui/mission/QGCMissionOther.cc \
    src/ui/mission/QGCMissionNavWaypoint.cc \
    src/ui/mission/QGCMissionDoJump.cc \
    src/ui/mission/QGCMissionConditionDelay.cc \
    src/ui/mission/QGCMissionNavLoiterUnlim.cc \
    src/ui/mission/QGCMissionNavLoiterTurns.cc \
    src/ui/mission/QGCMissionNavLoiterTime.cc \
    src/ui/mission/QGCMissionNavReturnToLaunch.cc \
    src/ui/mission/QGCMissionNavLand.cc \
    src/ui/mission/QGCMissionNavTakeoff.cc \
    src/ui/mission/QGCMissionNavSweep.cc \
    src/ui/mission/QGCMissionDoStartSearch.cc \
    src/ui/mission/QGCMissionDoFinishSearch.cc \
    src/ui/QGCHilConfiguration.cc \
    src/ui/QGCHilFlightGearConfiguration.cc \
    src/ui/QGCHilJSBSimConfiguration.cc \
    src/ui/QGCHilXPlaneConfiguration.cc \
    src/ui/uas/UASQuickViewItem.cc \
    src/ui/uas/UASQuickView.cc \
    src/ui/linechart/ChartPlot.cc \
    src/ui/uas/UASQuickViewTextItem.cc \
    src/ui/uas/UASQuickViewGaugeItem.cc \
    src/ui/uas/UASQuickViewItemSelect.cc \
    src/ui/QGCTabbedInfoView.cpp \
    src/ui/UASRawStatusView.cpp \
    src/ui/JoystickButton.cc \
    src/ui/JoystickAxis.cc \
    src/ui/uas/UASMessageView.cc \
    src/ui/configuration/terminalconsole.cpp \
    src/ui/configuration/console.cpp \
    src/ui/configuration/SerialSettingsDialog.cc \
    src/ui/configuration/ApmHighlighter.cc \
    src/uas/UASParameterDataModel.cc \
    src/uas/UASParameterCommsMgr.cc \
    src/ui/px4_configuration/PX4RCCalibration.cc \
    src/ui/px4_configuration/RCValueWidget.cc \
    src/uas/QGCUASFileManager.cc \
    src/ui/QGCUASFileView.cc \
    src/CmdLineOptParser.cc \
    src/QGCFileDialog.cc \
    src/QGCComboBox.cc \
    src/QGCTemporaryFile.cc \
    src/audio/QGCAudioWorker.cpp \
    src/QGCQuickWidget.cc \
    src/QGCPalette.cc \
    src/QGCQmlWidgetHolder.cpp \
    src/ui/QGCMapRCToParamDialog.cpp \
    src/QGCDockWidget.cc \
    src/ui/QGCLinkConfiguration.cc \
    src/comm/LinkConfiguration.cc \
    src/ui/QGCCommConfiguration.cc \
    src/ui/QGCUDPLinkConfiguration.cc \
    src/uas/UASMessageHandler.cc \
    src/ui/toolbar/MainToolBar.cc \
    src/QmlControls/ScreenTools.cc \
    src/QmlControls/ParameterEditorController.cc \
    src/QGCLoggingCategory.cc \
    src/ui/flightdisplay/QGCFlightDisplay.cc \
    src/ui/mapdisplay/QGCMapDisplay.cc \
    src/ViewWidgets/ViewWidgetController.cc \

#
# Unit Test specific configuration goes here
#
# We have to special case Windows debug_and_release builds because you can't have files
# which are only in the debug variant [QTBUG-40351]. So in this case we include unit tests
# even in the release variant. If you want a Windows release build with no unit tests run
# qmake with CONFIG-=debug_and_release CONFIG+=release.
#

DebugBuild|WindowsDebugAndRelease {

INCLUDEPATH += \
	src/qgcunittest

HEADERS += \
    src/qgcunittest/UnitTest.h \
    src/qgcunittest/MessageBoxTest.h \
    src/qgcunittest/FileDialogTest.h \
    src/qgcunittest/MockLink.h \
    src/qgcunittest/MockLinkMissionItemHandler.h \
	src/qgcunittest/MockUASManager.h \
	src/qgcunittest/MockUAS.h \
	src/qgcunittest/MockQGCUASParamManager.h \
	src/qgcunittest/MockMavlinkInterface.h \
	src/qgcunittest/MockMavlinkFileServer.h \
	src/qgcunittest/MultiSignalSpy.h \
	src/qgcunittest/FlightGearTest.h \
	src/qgcunittest/TCPLinkTest.h \
	src/qgcunittest/TCPLoopBackServer.h \
	src/qgcunittest/QGCUASFileManagerTest.h \
    src/qgcunittest/PX4RCCalibrationTest.h \
    src/qgcunittest/LinkManagerTest.h \
    src/qgcunittest/MainWindowTest.h \
    src/qgcunittest/MavlinkLogTest.h \
    src/FactSystem/FactSystemTestBase.h \
    src/FactSystem/FactSystemTestPX4.h \
    src/FactSystem/FactSystemTestGeneric.h \
    src/QmlControls/QmlTestWidget.h \
    src/VehicleSetup/SetupViewTest.h \

SOURCES += \
    src/qgcunittest/UnitTest.cc \
    src/qgcunittest/MessageBoxTest.cc \
    src/qgcunittest/FileDialogTest.cc \
    src/qgcunittest/MockLink.cc \
    src/qgcunittest/MockLinkMissionItemHandler.cc \
	src/qgcunittest/MockUASManager.cc \
	src/qgcunittest/MockUAS.cc \
	src/qgcunittest/MockQGCUASParamManager.cc \
	src/qgcunittest/MockMavlinkFileServer.cc \
	src/qgcunittest/MultiSignalSpy.cc \
	src/qgcunittest/FlightGearTest.cc \
	src/qgcunittest/TCPLinkTest.cc \
	src/qgcunittest/TCPLoopBackServer.cc \
	src/qgcunittest/QGCUASFileManagerTest.cc \
    src/qgcunittest/PX4RCCalibrationTest.cc \
    src/qgcunittest/LinkManagerTest.cc \
    src/qgcunittest/MainWindowTest.cc \
    src/qgcunittest/MavlinkLogTest.cc \
    src/FactSystem/FactSystemTestBase.cc \
    src/FactSystem/FactSystemTestPX4.cc \
    src/FactSystem/FactSystemTestGeneric.cc \
    src/QmlControls/QmlTestWidget.cc \
    src/VehicleSetup/SetupViewTest.cc \

}

#
# AutoPilot Plugin Support
#

INCLUDEPATH += \
    src/VehicleSetup

FORMS += \
    src/VehicleSetup/SetupView.ui \

HEADERS+= \
    src/VehicleSetup/SetupView.h \
    src/VehicleSetup/VehicleComponent.h \
    src/VehicleSetup/FirmwareUpgradeController.h \
    src/VehicleSetup/PX4Bootloader.h \
    src/VehicleSetup/PX4FirmwareUpgradeThread.h \
    src/AutoPilotPlugins/AutoPilotPluginManager.h \
    src/AutoPilotPlugins/AutoPilotPlugin.h \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.h \
    src/AutoPilotPlugins/Generic/GenericParameterFacts.h \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h \
    src/AutoPilotPlugins/PX4/PX4Component.h \
    src/AutoPilotPlugins/PX4/RadioComponent.h \
    src/AutoPilotPlugins/PX4/FlightModesComponent.h \
    src/AutoPilotPlugins/PX4/FlightModesComponentController.h \
    src/AutoPilotPlugins/PX4/AirframeComponent.h \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.h \
    src/AutoPilotPlugins/PX4/AirframeComponentController.h \
    src/AutoPilotPlugins/PX4/SensorsComponent.h \
    src/AutoPilotPlugins/PX4/SensorsComponentController.h \
    src/AutoPilotPlugins/PX4/SafetyComponent.h \
    src/AutoPilotPlugins/PX4/PowerComponent.h \
    src/AutoPilotPlugins/PX4/PX4ParameterFacts.h \

SOURCES += \
    src/VehicleSetup/SetupView.cc \
    src/VehicleSetup/VehicleComponent.cc \
    src/VehicleSetup/FirmwareUpgradeController.cc \
    src/VehicleSetup/PX4Bootloader.cc \
    src/VehicleSetup/PX4FirmwareUpgradeThread.cc \
    src/AutoPilotPlugins/AutoPilotPluginManager.cc \
    src/AutoPilotPlugins/AutoPilotPlugin.cc \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc \
    src/AutoPilotPlugins/Generic/GenericParameterFacts.cc \
    src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc \
    src/AutoPilotPlugins/PX4/PX4Component.cc \
    src/AutoPilotPlugins/PX4/RadioComponent.cc \
    src/AutoPilotPlugins/PX4/FlightModesComponent.cc \
    src/AutoPilotPlugins/PX4/FlightModesComponentController.cc \
    src/AutoPilotPlugins/PX4/AirframeComponent.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc \
    src/AutoPilotPlugins/PX4/AirframeComponentController.cc \
    src/AutoPilotPlugins/PX4/SensorsComponent.cc \
    src/AutoPilotPlugins/PX4/SensorsComponentController.cc \
    src/AutoPilotPlugins/PX4/SafetyComponent.cc \
    src/AutoPilotPlugins/PX4/PowerComponent.cc \
    src/AutoPilotPlugins/PX4/PX4ParameterFacts.cc \

# Fact System code

INCLUDEPATH += \
    src/FactSystem

HEADERS += \
    src/FactSystem/FactSystem.h \
    src/FactSystem/Fact.h \
    src/FactSystem/FactBinder.h \
    src/FactSystem/FactMetaData.h \
    src/FactSystem/FactValidator.h \
    src/FactSystem/ParameterLoader.h \

SOURCES += \
    src/FactSystem/FactSystem.cc \
    src/FactSystem/Fact.cc \
    src/FactSystem/FactBinder.cc \
    src/FactSystem/FactMetaData.cc \
    src/FactSystem/FactValidator.cc \
    src/FactSystem/ParameterLoader.cc \
