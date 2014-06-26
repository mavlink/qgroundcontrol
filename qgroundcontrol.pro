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

message(Qt version $$[QT_VERSION])

# Setup our supported build types. We do this once here and then use the defined config scopes
# to allow us to easily modify suported build types in one place instead of duplicated throughout
# the project file.

linux-g++ | linux-g++-64 {
    message(Linux build)
    CONFIG += LinuxBuild
} else : win32-msvc2008 | win32-msvc2010 | win32-msvc2012 {
    message(Windows build)
    CONFIG += WindowsBuild
} else : macx-clang | macx-llvm {
    message(Mac build)
    CONFIG += MacBuild
} else {
    error(Unsupported build type)
}

# Installer configuration

installer {
    CONFIG -= debug
    CONFIG -= debug_and_release
    CONFIG += release
    message(Build Installer)
}

# Setup our supported build flavors

CONFIG(debug, debug|release) {
    message(Debug flavor)
    CONFIG += DebugBuild
} else:CONFIG(release, debug|release) {
    message(Release flavor)
    CONFIG += ReleaseBuild
} else {
    error(Unsupported build flavor)
}

# Setup our build directories

BASEDIR = $${IN_PWD}
DebugBuild {
    DESTDIR = $${OUT_PWD}/debug
    BUILDDIR = $${OUT_PWD}/build-debug
}
ReleaseBuild {
    DESTDIR = $${OUT_PWD}/release
    BUILDDIR = $${OUT_PWD}/build-release
}
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
UI_DIR = $${BUILDDIR}/ui
RCC_DIR = $${BUILDDIR}/rcc
LANGUAGE = C++

message(BASEDIR $$BASEDIR DESTDIR $$DESTDIR TARGET $$TARGET)

# Qt configuration
CONFIG += qt \
    thread

QT += network \
    opengl \
    svg \
    xml \
    phonon \
    webkit \
    sql \
    declarative
        
#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

#
# OS Specific settings
#

MacBuild {
    QMAKE_INFO_PLIST = Custom-Info.plist
    CONFIG += x86_64
    CONFIG -= x86
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
	ICON = $$BASEDIR/files/images/icons/macx.icns
}

LinuxBuild {
	DEFINES += __STDC_LIMIT_MACROS
}

WindowsBuild {
	DEFINES += __STDC_LIMIT_MACROS

	# Specify multi-process compilation within Visual Studio.
	# (drastically improves compilation times for multi-core computers)
	QMAKE_CXXFLAGS_DEBUG += -MP
	QMAKE_CXXFLAGS_RELEASE += -MP

	# Specify that the Unicode versions of string functions should be used in the Windows API.
	# Without this the utils and qserialport libraries crash.
	DEFINES += UNICODE

	# QWebkit is not needed on MS-Windows compilation environment
	CONFIG -= webkit

	RC_FILE = $$BASEDIR/qgroundcontrol.rc
}

#
# By default warnings as errors are turned off. Even so, in order for a pull request 
# to be accepted you must compile cleanly with warnings as errors turned on the default 
# set of OS builds. See http://www.qgroundcontrol.org/dev/contribute for more details. 
# You can use the WarningsAsErrorsOn CONFIG switch to turn warnings as errors on for your 
# own builds.
#

MacBuild | LinuxBuild {
	QMAKE_CXXFLAGS_WARN_ON += -Wall
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += -Werror
    }
}

WindowsBuild {
	QMAKE_CXXFLAGS_WARN_ON += /W3 \
        /wd4996 \   # silence warnings about deprecated strcpy and whatnot
        /wd4005 \   # silence warnings about macro redefinition
        /wd4290     # ignore exception specifications
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += /WX
    }
}

#
# Build flavor specific settings
#

DebugBuild {
    CONFIG += console
}

ReleaseBuild {
    DEFINES += QT_NO_DEBUG

	WindowsBuild {
		# Use link time code generation for beteer optimization (I believe this is supported in msvc express, but not 100% sure)
		QMAKE_LFLAGS_LTCG = /LTCG
		QMAKE_CFLAGS_LTCG = -GL
    }
}

#
# Unit Test specific configuration goes here (debug only)
#

DebugBuild {
    INCLUDEPATH += \
        src/qgcunittest

    HEADERS += \
        src/qgcunittest/AutoTest.h \
        src/qgcunittest/UASUnitTest.h \
        src/qgcunittest/MockUASManager.h \
        src/qgcunittest/MockUAS.h \
        src/qgcunittest/MockQGCUASParamManager.h \
        src/qgcunittest/MultiSignalSpy.h \
        src/qgcunittest/FlightModeConfigTest.h \
        src/qgcunittest/FlightGearTest.h \
        src/qgcunittest/TCPLinkTest.h \
        src/qgcunittest/TCPLoopBackServer.h

    SOURCES += \
        src/qgcunittest/UASUnitTest.cc \
        src/qgcunittest/MockUASManager.cc \
        src/qgcunittest/MockUAS.cc \
        src/qgcunittest/MockQGCUASParamManager.cc \
        src/qgcunittest/MultiSignalSpy.cc \
        src/qgcunittest/FlightModeConfigTest.cc \
        src/qgcunittest/FlightGearTest.cc \
        src/qgcunittest/TCPLinkTest.cc \
        src/qgcunittest/TCPLoopBackServer.cc
}

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

TRANSLATIONS += \
    es-MX.ts \
    en-US.ts
    
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
    include/ui \
    src/input \
    src/lib/qmapcontrol \
    src/ui/mavlink \
    src/ui/param \
    src/ui/watchdog \
    src/ui/map3D \
    src/ui/mission \
    src/ui/designer \
    src/ui/configuration \
    src/ui/main

FORMS += \
    src/ui/MainWindow.ui \
    src/ui/CommSettings.ui \
    src/ui/SerialSettings.ui \
    src/ui/UASControl.ui \
    src/ui/UASList.ui \
    src/ui/UASInfo.ui \
    src/ui/Linechart.ui \
    src/ui/UASView.ui \
    src/ui/ParameterInterface.ui \
    src/ui/WaypointList.ui \
    src/ui/ObjectDetectionView.ui \
    src/ui/JoystickWidget.ui \
    src/ui/DebugConsole.ui \
    src/ui/HDDisplay.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/AudioOutputWidget.ui \
    src/ui/QGCSensorSettingsWidget.ui \
    src/ui/watchdog/WatchdogControl.ui \
    src/ui/watchdog/WatchdogProcessView.ui \
    src/ui/watchdog/WatchdogView.ui \
    src/ui/QGCFirmwareUpdate.ui \
    src/ui/QGCPxImuFirmwareUpdate.ui \
    src/ui/QGCDataPlot2D.ui \
    src/ui/QGCRemoteControlView.ui \
    src/ui/QMap3D.ui \
    src/ui/QGCWebView.ui \
    src/ui/map3D/QGCGoogleEarthView.ui \
    src/ui/SlugsDataSensorView.ui \
    src/ui/SlugsHilSim.ui \
    src/ui/SlugsPadCameraControl.ui \
    src/ui/uas/QGCUnconnectedInfoWidget.ui \
    src/ui/designer/QGCToolWidget.ui \
    src/ui/designer/QGCParamSlider.ui \
    src/ui/designer/QGCActionButton.ui \
    src/ui/designer/QGCCommandButton.ui \
    src/ui/QGCMAVLinkLogPlayer.ui \
    src/ui/QGCWaypointListMulti.ui \
    src/ui/QGCUASFileViewMulti.ui \
    src/ui/QGCUDPLinkConfiguration.ui \
    src/ui/QGCTCPLinkConfiguration.ui \
    src/ui/QGCSettingsWidget.ui \
    src/ui/UASControlParameters.ui \
    src/ui/map/QGCMapTool.ui \
    src/ui/map/QGCMapToolBar.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/WaypointViewOnlyView.ui \
    src/ui/WaypointEditableView.ui \
    src/ui/mavlink/QGCMAVLinkMessageSender.ui \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.ui \
    src/ui/QGCPluginHost.ui \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.ui \
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
    src/ui/QGCVehicleConfig.ui \
    src/ui/QGCPX4VehicleConfig.ui \
    src/ui/QGCHilConfiguration.ui \
    src/ui/QGCHilFlightGearConfiguration.ui \
    src/ui/QGCHilJSBSimConfiguration.ui \
    src/ui/QGCHilXPlaneConfiguration.ui \
    src/ui/designer/QGCComboBox.ui \
    src/ui/designer/QGCTextLabel.ui \
    src/ui/uas/UASQuickView.ui \
    src/ui/uas/UASQuickViewItemSelect.ui \
    src/ui/uas/UASActionsWidget.ui \
    src/ui/QGCTabbedInfoView.ui \
    src/ui/UASRawStatusView.ui \
    src/ui/uas/QGCMessageView.ui \
    src/ui/JoystickButton.ui \
    src/ui/JoystickAxis.ui \
    src/ui/configuration/ApmHardwareConfig.ui \
    src/ui/configuration/ApmSoftwareConfig.ui \
    src/ui/configuration/FrameTypeConfig.ui \
    src/ui/configuration/CompassConfig.ui \
    src/ui/configuration/AccelCalibrationConfig.ui \
    src/ui/configuration/RadioCalibrationConfig.ui \
    src/ui/configuration/FlightModeConfig.ui \
    src/ui/configuration/Radio3DRConfig.ui \
    src/ui/configuration/BatteryMonitorConfig.ui \
    src/ui/configuration/SonarConfig.ui \
    src/ui/configuration/AirspeedConfig.ui \
    src/ui/configuration/OpticalFlowConfig.ui \
    src/ui/configuration/OsdConfig.ui \
    src/ui/configuration/AntennaTrackerConfig.ui \
    src/ui/configuration/CameraGimbalConfig.ui \
    src/ui/configuration/BasicPidConfig.ui \
    src/ui/configuration/StandardParamConfig.ui \
    src/ui/configuration/GeoFenceConfig.ui \
    src/ui/configuration/FailSafeConfig.ui \
    src/ui/configuration/AdvancedParamConfig.ui \
    src/ui/configuration/ArduCopterPidConfig.ui \
    src/ui/configuration/ApmPlaneLevel.ui \
    src/ui/configuration/ParamWidget.ui \
    src/ui/configuration/ArduPlanePidConfig.ui \
    src/ui/configuration/AdvParameterList.ui \
    src/ui/configuration/ArduRoverPidConfig.ui \
    src/ui/QGCConfigView.ui \
    src/ui/main/QGCViewModeSelection.ui \
    src/ui/main/QGCWelcomeMainWindow.ui \
    src/ui/configuration/terminalconsole.ui \
    src/ui/configuration/SerialSettingsDialog.ui \
    src/ui/configuration/ApmFirmwareConfig.ui \
    src/ui/px4_configuration/QGCPX4AirframeConfig.ui \
    src/ui/px4_configuration/QGCPX4MulticopterConfig.ui \
    src/ui/px4_configuration/QGCPX4SensorCalibration.ui \
    src/ui/designer/QGCXYPlot.ui \
    src/ui/QGCUASFileView.ui

HEADERS += \
    src/MG.h \
    src/QGCCore.h \
    src/uas/UASInterface.h \
    src/uas/UAS.h \
    src/uas/UASManager.h \
    src/comm/LinkManager.h \
    src/comm/LinkInterface.h \
    src/comm/SerialLinkInterface.h \
    src/comm/SerialLink.h \
    src/comm/ProtocolInterface.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/QGCFlightGearLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
    src/ui/CommConfigurationWindow.h \
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
    src/configuration.h \
    src/ui/uas/UASView.h \
    src/ui/CameraView.h \
    src/comm/MAVLinkSimulationLink.h \
    src/comm/UDPLink.h \
    src/comm/TCPLink.h \
    src/ui/ParameterInterface.h \
    src/ui/WaypointList.h \
    src/Waypoint.h \
    src/ui/ObjectDetectionView.h \
    src/input/JoystickInput.h \
    src/ui/JoystickWidget.h \
    src/ui/DebugConsole.h \
    src/ui/HDDisplay.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/ui/AudioOutputWidget.h \
    src/GAudioOutput.h \
    src/LogCompressor.h \
    src/ui/QGCParamWidget.h \
    src/ui/QGCSensorSettingsWidget.h \
    src/ui/linechart/Linecharts.h \
    src/uas/SlugsMAV.h \
    src/uas/PxQuadMAV.h \
    src/uas/ArduPilotMegaMAV.h \
    src/uas/senseSoarMAV.h \
    src/ui/watchdog/WatchdogControl.h \
    src/ui/watchdog/WatchdogProcessView.h \
    src/ui/watchdog/WatchdogView.h \
    src/uas/UASWaypointManager.h \
    src/ui/HSIDisplay.h \
    src/QGC.h \
    src/ui/QGCFirmwareUpdate.h \
    src/ui/QGCPxImuFirmwareUpdate.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/linechart/IncrementalPlot.h \
    src/ui/QGCRemoteControlView.h \
    src/ui/RadioCalibration/RadioCalibrationData.h \
    src/ui/RadioCalibration/RadioCalibrationWindow.h \
    src/ui/RadioCalibration/AirfoilServoCalibrator.h \
    src/ui/RadioCalibration/SwitchCalibrator.h \
    src/ui/RadioCalibration/CurveCalibrator.h \
    src/ui/RadioCalibration/AbstractCalibrator.h \
    src/comm/QGCMAVLink.h \
    src/ui/QGCWebView.h \
    src/ui/map3D/QGCWebPage.h \
    src/ui/SlugsDataSensorView.h \
    src/ui/SlugsHilSim.h \
    src/ui/SlugsPadCameraControl.h \
    src/ui/QGCMainWindowAPConfigurator.h \
    src/comm/MAVLinkSwarmSimulationLink.h \
    src/ui/uas/QGCUnconnectedInfoWidget.h \
    src/ui/designer/QGCToolWidget.h \
    src/ui/designer/QGCParamSlider.h \
    src/ui/designer/QGCCommandButton.h \
    src/ui/designer/QGCToolWidgetItem.h \
    src/ui/QGCMAVLinkLogPlayer.h \
    src/comm/MAVLinkSimulationWaypointPlanner.h \
    src/comm/MAVLinkSimulationMAV.h \
    src/uas/QGCMAVLinkUASFactory.h \
    src/ui/QGCWaypointListMulti.h \
    src/ui/QGCUASFileViewMulti.h \
    src/ui/QGCUDPLinkConfiguration.h \
    src/ui/QGCTCPLinkConfiguration.h \
    src/ui/QGCSettingsWidget.h \
    src/ui/uas/UASControlParameters.h \
    src/uas/QGCUASParamManager.h \
    src/ui/map/QGCMapWidget.h \
    src/ui/map/MAV2DIcon.h \
    src/ui/map/Waypoint2DIcon.h \
    src/ui/map/QGCMapTool.h \
    src/ui/map/QGCMapToolBar.h \
    src/QGCGeo.h \
    src/ui/QGCToolBar.h \
    src/ui/QGCStatusBar.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/WaypointViewOnlyView.h \
    src/ui/WaypointEditableView.h \
    src/ui/QGCRGBDView.h \
    src/ui/mavlink/QGCMAVLinkMessageSender.h \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.h \
    src/ui/QGCPluginHost.h \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.h \
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
    src/ui/QGCVehicleConfig.h \
    src/ui/QGCPX4VehicleConfig.h \
    src/comm/QGCHilLink.h \
    src/ui/QGCHilConfiguration.h \
    src/ui/QGCHilFlightGearConfiguration.h \
    src/ui/QGCHilJSBSimConfiguration.h \
    src/ui/QGCHilXPlaneConfiguration.h \
    src/ui/designer/QGCComboBox.h \
    src/ui/designer/QGCTextLabel.h \
    src/ui/submainwindow.h \
    src/ui/uas/UASQuickView.h \
    src/ui/uas/UASQuickViewItem.h \
    src/ui/linechart/ChartPlot.h \
    src/ui/uas/UASQuickViewItemSelect.h \
    src/ui/uas/UASQuickViewTextItem.h \
    src/ui/uas/UASQuickViewGaugeItem.h \
    src/ui/uas/UASActionsWidget.h \
    src/ui/designer/QGCRadioChannelDisplay.h \
    src/ui/QGCTabbedInfoView.h \
    src/ui/UASRawStatusView.h \
    src/ui/PrimaryFlightDisplay.h \
    src/ui/uas/QGCMessageView.h \
    src/ui/JoystickButton.h \
    src/ui/JoystickAxis.h \
    src/ui/configuration/ApmHardwareConfig.h \
    src/ui/configuration/ApmSoftwareConfig.h \
    src/ui/configuration/FrameTypeConfig.h \
    src/ui/configuration/CompassConfig.h \
    src/ui/configuration/AccelCalibrationConfig.h \
    src/ui/configuration/RadioCalibrationConfig.h \
    src/ui/configuration/FlightModeConfig.h \
    src/ui/configuration/Radio3DRConfig.h \
    src/ui/configuration/BatteryMonitorConfig.h \
    src/ui/configuration/SonarConfig.h \
    src/ui/configuration/AirspeedConfig.h \
    src/ui/configuration/OpticalFlowConfig.h \
    src/ui/configuration/OsdConfig.h \
    src/ui/configuration/AntennaTrackerConfig.h \
    src/ui/configuration/CameraGimbalConfig.h \
    src/ui/configuration/AP2ConfigWidget.h \
    src/ui/configuration/BasicPidConfig.h \
    src/ui/configuration/StandardParamConfig.h \
    src/ui/configuration/GeoFenceConfig.h \
    src/ui/configuration/FailSafeConfig.h \
    src/ui/configuration/AdvancedParamConfig.h \
    src/ui/configuration/ArduCopterPidConfig.h \
    src/ui/apmtoolbar.h \
    src/ui/configuration/ApmPlaneLevel.h \
    src/ui/configuration/ParamWidget.h \
    src/ui/configuration/ArduPlanePidConfig.h \
    src/ui/configuration/AdvParameterList.h \
    src/ui/configuration/ArduRoverPidConfig.h \
    src/ui/QGCConfigView.h \
    src/ui/main/QGCViewModeSelection.h \
    src/ui/main/QGCWelcomeMainWindow.h \
    src/ui/configuration/console.h \
    src/ui/configuration/SerialSettingsDialog.h \
    src/ui/configuration/terminalconsole.h \
    src/ui/configuration/ApmHighlighter.h \
    src/ui/configuration/ApmFirmwareConfig.h \
    src/uas/UASParameterDataModel.h \
    src/uas/UASParameterCommsMgr.h \
    src/ui/QGCPendingParamWidget.h \
    src/ui/px4_configuration/QGCPX4AirframeConfig.h \
    src/ui/QGCBaseParamWidget.h \
    src/ui/px4_configuration/QGCPX4MulticopterConfig.h \
    src/ui/px4_configuration/QGCPX4SensorCalibration.h \
    src/ui/designer/QGCXYPlot.h \
    src/ui/menuactionhelper.h \
    src/uas/UASManagerInterface.h \
    src/uas/QGCUASParamManagerInterface.h \
    src/uas/QGCUASFileManager.h \
    src/ui/QGCUASFileView.h \
    src/uas/QGCUASWorker.h \
    src/CmdLineOptParser.h \
    src/uas/QGXPX4UAS.h

SOURCES += \
    src/main.cc \
    src/QGCCore.cc \
    src/uas/UASManager.cc \
    src/uas/UAS.cc \
    src/comm/LinkManager.cc \
    src/comm/SerialLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/QGCFlightGearLink.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
    src/ui/CommConfigurationWindow.cc \
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
    src/ui/ParameterInterface.cc \
    src/ui/WaypointList.cc \
    src/Waypoint.cc \
    src/ui/ObjectDetectionView.cc \
    src/input/JoystickInput.cc \
    src/ui/JoystickWidget.cc \
    src/ui/DebugConsole.cc \
    src/ui/HDDisplay.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/ui/AudioOutputWidget.cc \
    src/GAudioOutput.cc \
    src/LogCompressor.cc \
    src/ui/QGCParamWidget.cc \
    src/ui/QGCSensorSettingsWidget.cc \
    src/ui/linechart/Linecharts.cc \
    src/uas/SlugsMAV.cc \
    src/uas/PxQuadMAV.cc \
    src/uas/ArduPilotMegaMAV.cc \
    src/uas/senseSoarMAV.cpp \
    src/ui/watchdog/WatchdogControl.cc \
    src/ui/watchdog/WatchdogProcessView.cc \
    src/ui/watchdog/WatchdogView.cc \
    src/uas/UASWaypointManager.cc \
    src/ui/HSIDisplay.cc \
    src/QGC.cc \
    src/ui/QGCFirmwareUpdate.cc \
    src/ui/QGCPxImuFirmwareUpdate.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/QGCRemoteControlView.cc \
    src/ui/RadioCalibration/RadioCalibrationWindow.cc \
    src/ui/RadioCalibration/AirfoilServoCalibrator.cc \
    src/ui/RadioCalibration/SwitchCalibrator.cc \
    src/ui/RadioCalibration/CurveCalibrator.cc \
    src/ui/RadioCalibration/AbstractCalibrator.cc \
    src/ui/RadioCalibration/RadioCalibrationData.cc \
    src/ui/QGCWebView.cc \
    src/ui/map3D/QGCWebPage.cc \
    src/ui/SlugsDataSensorView.cc \
    src/ui/SlugsHilSim.cc \
    src/ui/SlugsPadCameraControl.cpp \
    src/ui/QGCMainWindowAPConfigurator.cc \
    src/comm/MAVLinkSwarmSimulationLink.cc \
    src/ui/uas/QGCUnconnectedInfoWidget.cc \
    src/ui/designer/QGCToolWidget.cc \
    src/ui/designer/QGCParamSlider.cc \
    src/ui/designer/QGCCommandButton.cc \
    src/ui/designer/QGCToolWidgetItem.cc \
    src/ui/QGCMAVLinkLogPlayer.cc \
    src/comm/MAVLinkSimulationWaypointPlanner.cc \
    src/comm/MAVLinkSimulationMAV.cc \
    src/uas/QGCMAVLinkUASFactory.cc \
    src/ui/QGCWaypointListMulti.cc \
    src/ui/QGCUASFileViewMulti.cc \
    src/ui/QGCUDPLinkConfiguration.cc \
    src/ui/QGCTCPLinkConfiguration.cc \
    src/ui/QGCSettingsWidget.cc \
    src/ui/uas/UASControlParameters.cpp \
    src/uas/QGCUASParamManager.cc \
    src/ui/map/QGCMapWidget.cc \
    src/ui/map/MAV2DIcon.cc \
    src/ui/map/Waypoint2DIcon.cc \
    src/ui/map/QGCMapTool.cc \
    src/ui/map/QGCMapToolBar.cc \
    src/ui/QGCToolBar.cc \
    src/ui/QGCStatusBar.cc \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/WaypointViewOnlyView.cc \
    src/ui/WaypointEditableView.cc \
    src/ui/QGCRGBDView.cc \
    src/ui/mavlink/QGCMAVLinkMessageSender.cc \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.cc \
    src/ui/QGCPluginHost.cc \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.cc \
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
    src/ui/QGCVehicleConfig.cc \
    src/ui/QGCPX4VehicleConfig.cc \
    src/ui/QGCHilConfiguration.cc \
    src/ui/QGCHilFlightGearConfiguration.cc \
    src/ui/QGCHilJSBSimConfiguration.cc \
    src/ui/QGCHilXPlaneConfiguration.cc \
    src/ui/designer/QGCComboBox.cc \
    src/ui/designer/QGCTextLabel.cc \
    src/ui/submainwindow.cpp \
    src/ui/uas/UASQuickViewItem.cc \
    src/ui/uas/UASQuickView.cc \
    src/ui/linechart/ChartPlot.cc \
    src/ui/uas/UASQuickViewTextItem.cc \
    src/ui/uas/UASQuickViewGaugeItem.cc \
    src/ui/uas/UASQuickViewItemSelect.cc \
    src/ui/uas/UASActionsWidget.cpp \
    src/ui/designer/QGCRadioChannelDisplay.cpp \
    src/ui/QGCTabbedInfoView.cpp \
    src/ui/UASRawStatusView.cpp \
    src/ui/PrimaryFlightDisplay.cc \
    src/ui/JoystickButton.cc \
    src/ui/JoystickAxis.cc \
    src/ui/uas/QGCMessageView.cc \
    src/ui/configuration/ApmHardwareConfig.cc \
    src/ui/configuration/ApmSoftwareConfig.cc \
    src/ui/configuration/FrameTypeConfig.cc \
    src/ui/configuration/CompassConfig.cc \
    src/ui/configuration/AccelCalibrationConfig.cc \
    src/ui/configuration/RadioCalibrationConfig.cc \
    src/ui/configuration/FlightModeConfig.cc \
    src/ui/configuration/Radio3DRConfig.cc \
    src/ui/configuration/BatteryMonitorConfig.cc \
    src/ui/configuration/SonarConfig.cc \
    src/ui/configuration/AirspeedConfig.cc \
    src/ui/configuration/OpticalFlowConfig.cc \
    src/ui/configuration/OsdConfig.cc \
    src/ui/configuration/AntennaTrackerConfig.cc \
    src/ui/configuration/CameraGimbalConfig.cc \
    src/ui/configuration/AP2ConfigWidget.cc \
    src/ui/configuration/BasicPidConfig.cc \
    src/ui/configuration/StandardParamConfig.cc \
    src/ui/configuration/GeoFenceConfig.cc \
    src/ui/configuration/FailSafeConfig.cc \
    src/ui/configuration/AdvancedParamConfig.cc \
    src/ui/configuration/ArduCopterPidConfig.cc \
    src/ui/apmtoolbar.cpp \
    src/ui/configuration/ApmPlaneLevel.cc \
    src/ui/configuration/ParamWidget.cc \
    src/ui/configuration/ArduPlanePidConfig.cc \
    src/ui/configuration/AdvParameterList.cc \
    src/ui/configuration/ArduRoverPidConfig.cc \
    src/ui/QGCConfigView.cc \
    src/ui/main/QGCViewModeSelection.cc \
    src/ui/main/QGCWelcomeMainWindow.cc \
    src/ui/configuration/terminalconsole.cpp \
    src/ui/configuration/console.cpp \
    src/ui/configuration/SerialSettingsDialog.cc \
    src/ui/configuration/ApmHighlighter.cc \
    src/ui/configuration/ApmFirmwareConfig.cc \
    src/uas/UASParameterDataModel.cc \
    src/uas/UASParameterCommsMgr.cc \
    src/ui/QGCPendingParamWidget.cc \
    src/ui/px4_configuration/QGCPX4AirframeConfig.cc \
    src/ui/QGCBaseParamWidget.cc \
    src/ui/px4_configuration/QGCPX4MulticopterConfig.cc \
    src/ui/px4_configuration/QGCPX4SensorCalibration.cc \
    src/ui/designer/QGCXYPlot.cc \
    src/ui/menuactionhelper.cpp \
    src/uas/QGCUASFileManager.cc \
    src/ui/QGCUASFileView.cc \
    src/uas/QGCUASWorker.cc \
    src/CmdLineOptParser.cc \
    src/uas/QGXPX4UAS.cc
