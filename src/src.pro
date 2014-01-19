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
} else : macx-clang {
    message(Mac build)
    CONFIG += MacBuild
} else {
    error(Unsupported build type)
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
# Uncomment for Qt5 (which is when clean_path becomes available)
#BASEDIR = $$clean_path($${PWD}/..)
BASEDIR = $${PWD}/..
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

# Name our executable
TARGET = qgroundcontrol

message(BASEDIR=$$BASEDIR DESTDIR=$$DESTDIR TARGET=$$TARGET)

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

	# QWebkit is not needed on MS-Windows compilation environment
	CONFIG -= webkit

	RC_FILE = $$BASEDIR/qgroundcontrol.rc
}

#
# Warnings cleanup. Plan of attack is to turn off all existing warnings and turn on warnings as errors.
# Then we will clean up the warnings one type at a time, removing the override for that specific warning
# from the lists below. Eventually we will be left with no overlooked warnings and all future warnings
# generating an error and breaking the build.
#
# NEW WARNINGS SHOULD NOT BE ADDED TO THIS LIST. IF YOU GET AN ERROR, FIX IT BEFORE COMMITING.
#

MacBuild | LinuxBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wall \
        -Wno-unused-parameter \
        -Wno-unused-variable \
        -Wno-narrowing \
        -Wno-unused-function
 }

LinuxBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wno-unused-but-set-variable \
        -Wno-unused-local-typedefs
}

MacBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        -Wno-overloaded-virtual \
        -Wno-unused-private-field
}

WindowsBuild {
	QMAKE_CXXFLAGS_WARN_ON += \
        /W4 \
        /WX \
        /wd4005 \ # macro redefinition
        /wd4100 \ # unrefernced formal parameter
        /wd4101 \ # unreference local variable
        /wd4127 \ # conditional expression constant
        /wd4146 \ # unary minus operator applied to unsigned type
        /wd4189 \ # local variable initialized but not used
        /wd4201 \ # non standard extension: nameless struct/union
        /wd4245 \ # signed/unsigned mismtach
        /wd4290 \ # function declared using exception specification, but not supported
        /wd4305 \ # truncation from double to float
        /wd4309 \ # truncation of constant value
        /wd4389 \ # == signed/unsigned mismatch
        /wd4505 \ # unreferenced local function
        /wd4512 \ # assignment operation could not be generated
        /wd4701 \ # potentially uninitialized local variable
        /wd4702 \ # unreachable code
        /wd4996   # deprecated function
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
		# Use link time code generation for better optimization (I believe this is supported in msvc express, but not 100% sure)
		QMAKE_LFLAGS_LTCG = /LTCG
		QMAKE_CFLAGS_LTCG = -GL
    }
}

#
# Unit Test specific configuration goes here (debug only)
#

DebugBuild {
    INCLUDEPATH += \
        qgcunittest

    HEADERS += \
        qgcunittest/AutoTest.h \
        qgcunittest/UASUnitTest.h \
        qgcunittest/MockUASManager.h \
        qgcunittest/MockUAS.h \
        qgcunittest/MockQGCUASParamManager.h \
        qgcunittest/MultiSignalSpy.h \
        qgcunittest/TCPLinkTest.h \
        qgcunittest/FlightModeConfigTest.h

    SOURCES += \
        qgcunittest/UASUnitTest.cc \
        qgcunittest/MockUASManager.cc \
        qgcunittest/MockUAS.cc \
        qgcunittest/MockQGCUASParamManager.cc \
        qgcunittest/MultiSignalSpy.cc \
        qgcunittest/TCPLinkTest.cc \
        qgcunittest/FlightModeConfigTest.cc
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
    ui \
    ui/linechart \
    ui/uas \
    ui/map \
    uas \
    comm \
    include/ui \
    input \
    lib/qmapcontrol \
    ui/mavlink \
    ui/param \
    ui/watchdog \
    ui/map3D \
    ui/mission \
    ui/designer \
    ui/configuration \
    ui/main

FORMS += \
    ui/MainWindow.ui \
    ui/CommSettings.ui \
    ui/SerialSettings.ui \
    ui/UASControl.ui \
    ui/UASList.ui \
    ui/UASInfo.ui \
    ui/Linechart.ui \
    ui/UASView.ui \
    ui/ParameterInterface.ui \
    ui/WaypointList.ui \
    ui/ObjectDetectionView.ui \
    ui/JoystickWidget.ui \
    ui/DebugConsole.ui \
    ui/HDDisplay.ui \
    ui/MAVLinkSettingsWidget.ui \
    ui/AudioOutputWidget.ui \
    ui/QGCSensorSettingsWidget.ui \
    ui/watchdog/WatchdogControl.ui \
    ui/watchdog/WatchdogProcessView.ui \
    ui/watchdog/WatchdogView.ui \
    ui/QGCFirmwareUpdate.ui \
    ui/QGCPxImuFirmwareUpdate.ui \
    ui/QGCDataPlot2D.ui \
    ui/QGCRemoteControlView.ui \
    ui/QMap3D.ui \
    ui/QGCWebView.ui \
    ui/map3D/QGCGoogleEarthView.ui \
    ui/SlugsDataSensorView.ui \
    ui/SlugsHilSim.ui \
    ui/SlugsPadCameraControl.ui \
    ui/uas/QGCUnconnectedInfoWidget.ui \
    ui/designer/QGCToolWidget.ui \
    ui/designer/QGCParamSlider.ui \
    ui/designer/QGCActionButton.ui \
    ui/designer/QGCCommandButton.ui \
    ui/QGCMAVLinkLogPlayer.ui \
    ui/QGCWaypointListMulti.ui \
    ui/QGCUDPLinkConfiguration.ui \
    ui/QGCTCPLinkConfiguration.ui \
    ui/QGCSettingsWidget.ui \
    ui/UASControlParameters.ui \
    ui/map/QGCMapTool.ui \
    ui/map/QGCMapToolBar.ui \
    ui/QGCMAVLinkInspector.ui \
    ui/WaypointViewOnlyView.ui \
    ui/WaypointEditableView.ui \
    ui/mavlink/QGCMAVLinkMessageSender.ui \
    ui/firmwareupdate/QGCFirmwareUpdateWidget.ui \
    ui/QGCPluginHost.ui \
    ui/firmwareupdate/QGCPX4FirmwareUpdate.ui \
    ui/mission/QGCMissionOther.ui \
    ui/mission/QGCMissionNavWaypoint.ui \
    ui/mission/QGCMissionDoJump.ui \
    ui/mission/QGCMissionConditionDelay.ui \
    ui/mission/QGCMissionNavLoiterUnlim.ui \
    ui/mission/QGCMissionNavLoiterTurns.ui \
    ui/mission/QGCMissionNavLoiterTime.ui \
    ui/mission/QGCMissionNavReturnToLaunch.ui \
    ui/mission/QGCMissionNavLand.ui \
    ui/mission/QGCMissionNavTakeoff.ui \
    ui/mission/QGCMissionNavSweep.ui \
    ui/mission/QGCMissionDoStartSearch.ui \
    ui/mission/QGCMissionDoFinishSearch.ui \
    ui/QGCVehicleConfig.ui \
    ui/QGCPX4VehicleConfig.ui \
    ui/QGCHilConfiguration.ui \
    ui/QGCHilFlightGearConfiguration.ui \
    ui/QGCHilJSBSimConfiguration.ui \
    ui/QGCHilXPlaneConfiguration.ui \
    ui/designer/QGCComboBox.ui \
    ui/designer/QGCTextLabel.ui \
    ui/uas/UASQuickView.ui \
    ui/uas/UASQuickViewItemSelect.ui \
    ui/uas/UASActionsWidget.ui \
    ui/QGCTabbedInfoView.ui \
    ui/UASRawStatusView.ui \
    ui/uas/QGCMessageView.ui \
    ui/JoystickButton.ui \
    ui/JoystickAxis.ui \
    ui/configuration/ApmHardwareConfig.ui \
    ui/configuration/ApmSoftwareConfig.ui \
    ui/configuration/FrameTypeConfig.ui \
    ui/configuration/CompassConfig.ui \
    ui/configuration/AccelCalibrationConfig.ui \
    ui/configuration/RadioCalibrationConfig.ui \
    ui/configuration/FlightModeConfig.ui \
    ui/configuration/Radio3DRConfig.ui \
    ui/configuration/BatteryMonitorConfig.ui \
    ui/configuration/SonarConfig.ui \
    ui/configuration/AirspeedConfig.ui \
    ui/configuration/OpticalFlowConfig.ui \
    ui/configuration/OsdConfig.ui \
    ui/configuration/AntennaTrackerConfig.ui \
    ui/configuration/CameraGimbalConfig.ui \
    ui/configuration/BasicPidConfig.ui \
    ui/configuration/StandardParamConfig.ui \
    ui/configuration/GeoFenceConfig.ui \
    ui/configuration/FailSafeConfig.ui \
    ui/configuration/AdvancedParamConfig.ui \
    ui/configuration/ArduCopterPidConfig.ui \
    ui/configuration/ApmPlaneLevel.ui \
    ui/configuration/ParamWidget.ui \
    ui/configuration/ArduPlanePidConfig.ui \
    ui/configuration/AdvParameterList.ui \
    ui/configuration/ArduRoverPidConfig.ui \
    ui/QGCConfigView.ui \
    ui/main/QGCViewModeSelection.ui \
    ui/main/QGCWelcomeMainWindow.ui \
    ui/configuration/terminalconsole.ui \
    ui/configuration/SerialSettingsDialog.ui \
    ui/configuration/ApmFirmwareConfig.ui \
    ui/px4_configuration/QGCPX4AirframeConfig.ui \
    ui/px4_configuration/QGCPX4MulticopterConfig.ui \
    ui/px4_configuration/QGCPX4SensorCalibration.ui \
    ui/designer/QGCXYPlot.ui

HEADERS += \
    MG.h \
    QGCCore.h \
    uas/UASInterface.h \
    uas/UAS.h \
    uas/UASManager.h \
    comm/LinkManager.h \
    comm/LinkInterface.h \
    comm/SerialLinkInterface.h \
    comm/SerialLink.h \
    comm/ProtocolInterface.h \
    comm/MAVLinkProtocol.h \
    comm/QGCFlightGearLink.h \
    comm/QGCJSBSimLink.h \
    comm/QGCXPlaneLink.h \
    ui/CommConfigurationWindow.h \
    ui/SerialConfigurationWindow.h \
    ui/MainWindow.h \
    ui/uas/UASControlWidget.h \
    ui/uas/UASListWidget.h \
    ui/uas/UASInfoWidget.h \
    ui/HUD.h \
    ui/linechart/LinechartWidget.h \
    ui/linechart/LinechartPlot.h \
    ui/linechart/Scrollbar.h \
    ui/linechart/ScrollZoomer.h \
    configuration.h \
    ui/uas/UASView.h \
    ui/CameraView.h \
    comm/MAVLinkSimulationLink.h \
    comm/UDPLink.h \
    comm/TCPLink.h \
    ui/ParameterInterface.h \
    ui/WaypointList.h \
    Waypoint.h \
    ui/ObjectDetectionView.h \
    input/JoystickInput.h \
    ui/JoystickWidget.h \
    ui/DebugConsole.h \
    ui/HDDisplay.h \
    ui/MAVLinkSettingsWidget.h \
    ui/AudioOutputWidget.h \
    GAudioOutput.h \
    LogCompressor.h \
    ui/QGCParamWidget.h \
    ui/QGCSensorSettingsWidget.h \
    ui/linechart/Linecharts.h \
    uas/SlugsMAV.h \
    uas/PxQuadMAV.h \
    uas/ArduPilotMegaMAV.h \
    uas/senseSoarMAV.h \
    ui/watchdog/WatchdogControl.h \
    ui/watchdog/WatchdogProcessView.h \
    ui/watchdog/WatchdogView.h \
    uas/UASWaypointManager.h \
    ui/HSIDisplay.h \
    QGC.h \
    ui/QGCFirmwareUpdate.h \
    ui/QGCPxImuFirmwareUpdate.h \
    ui/QGCDataPlot2D.h \
    ui/linechart/IncrementalPlot.h \
    ui/QGCRemoteControlView.h \
    ui/RadioCalibration/RadioCalibrationData.h \
    ui/RadioCalibration/RadioCalibrationWindow.h \
    ui/RadioCalibration/AirfoilServoCalibrator.h \
    ui/RadioCalibration/SwitchCalibrator.h \
    ui/RadioCalibration/CurveCalibrator.h \
    ui/RadioCalibration/AbstractCalibrator.h \
    comm/QGCMAVLink.h \
    ui/QGCWebView.h \
    ui/map3D/QGCWebPage.h \
    ui/SlugsDataSensorView.h \
    ui/SlugsHilSim.h \
    ui/SlugsPadCameraControl.h \
    ui/QGCMainWindowAPConfigurator.h \
    comm/MAVLinkSwarmSimulationLink.h \
    ui/uas/QGCUnconnectedInfoWidget.h \
    ui/designer/QGCToolWidget.h \
    ui/designer/QGCParamSlider.h \
    ui/designer/QGCCommandButton.h \
    ui/designer/QGCToolWidgetItem.h \
    ui/QGCMAVLinkLogPlayer.h \
    comm/MAVLinkSimulationWaypointPlanner.h \
    comm/MAVLinkSimulationMAV.h \
    uas/QGCMAVLinkUASFactory.h \
    ui/QGCWaypointListMulti.h \
    ui/QGCUDPLinkConfiguration.h \
    ui/QGCTCPLinkConfiguration.h \
    ui/QGCSettingsWidget.h \
    ui/uas/UASControlParameters.h \
    uas/QGCUASParamManager.h \
    ui/map/QGCMapWidget.h \
    ui/map/MAV2DIcon.h \
    ui/map/Waypoint2DIcon.h \
    ui/map/QGCMapTool.h \
    ui/map/QGCMapToolBar.h \
    QGCGeo.h \
    ui/QGCToolBar.h \
    ui/QGCStatusBar.h \
    ui/QGCMAVLinkInspector.h \
    ui/MAVLinkDecoder.h \
    ui/WaypointViewOnlyView.h \
    ui/WaypointEditableView.h \
    ui/QGCRGBDView.h \
    ui/mavlink/QGCMAVLinkMessageSender.h \
    ui/firmwareupdate/QGCFirmwareUpdateWidget.h \
    ui/QGCPluginHost.h \
    ui/firmwareupdate/QGCPX4FirmwareUpdate.h \
    ui/mission/QGCMissionOther.h \
    ui/mission/QGCMissionNavWaypoint.h \
    ui/mission/QGCMissionDoJump.h \
    ui/mission/QGCMissionConditionDelay.h \
    ui/mission/QGCMissionNavLoiterUnlim.h \
    ui/mission/QGCMissionNavLoiterTurns.h \
    ui/mission/QGCMissionNavLoiterTime.h \
    ui/mission/QGCMissionNavReturnToLaunch.h \
    ui/mission/QGCMissionNavLand.h \
    ui/mission/QGCMissionNavTakeoff.h \
    ui/mission/QGCMissionNavSweep.h \
    ui/mission/QGCMissionDoStartSearch.h \
    ui/mission/QGCMissionDoFinishSearch.h \
    ui/QGCVehicleConfig.h \
    ui/QGCPX4VehicleConfig.h \
    comm/QGCHilLink.h \
    ui/QGCHilConfiguration.h \
    ui/QGCHilFlightGearConfiguration.h \
    ui/QGCHilJSBSimConfiguration.h \
    ui/QGCHilXPlaneConfiguration.h \
    ui/designer/QGCComboBox.h \
    ui/designer/QGCTextLabel.h \
    ui/submainwindow.h \
    ui/uas/UASQuickView.h \
    ui/uas/UASQuickViewItem.h \
    ui/linechart/ChartPlot.h \
    ui/uas/UASQuickViewItemSelect.h \
    ui/uas/UASQuickViewTextItem.h \
    ui/uas/UASQuickViewGaugeItem.h \
    ui/uas/UASActionsWidget.h \
    ui/designer/QGCRadioChannelDisplay.h \
    ui/QGCTabbedInfoView.h \
    ui/UASRawStatusView.h \
    ui/PrimaryFlightDisplay.h \
    ui/uas/QGCMessageView.h \
    ui/JoystickButton.h \
    ui/JoystickAxis.h \
    ui/configuration/ApmHardwareConfig.h \
    ui/configuration/ApmSoftwareConfig.h \
    ui/configuration/FrameTypeConfig.h \
    ui/configuration/CompassConfig.h \
    ui/configuration/AccelCalibrationConfig.h \
    ui/configuration/RadioCalibrationConfig.h \
    ui/configuration/FlightModeConfig.h \
    ui/configuration/Radio3DRConfig.h \
    ui/configuration/BatteryMonitorConfig.h \
    ui/configuration/SonarConfig.h \
    ui/configuration/AirspeedConfig.h \
    ui/configuration/OpticalFlowConfig.h \
    ui/configuration/OsdConfig.h \
    ui/configuration/AntennaTrackerConfig.h \
    ui/configuration/CameraGimbalConfig.h \
    ui/configuration/AP2ConfigWidget.h \
    ui/configuration/BasicPidConfig.h \
    ui/configuration/StandardParamConfig.h \
    ui/configuration/GeoFenceConfig.h \
    ui/configuration/FailSafeConfig.h \
    ui/configuration/AdvancedParamConfig.h \
    ui/configuration/ArduCopterPidConfig.h \
    ui/apmtoolbar.h \
    ui/configuration/ApmPlaneLevel.h \
    ui/configuration/ParamWidget.h \
    ui/configuration/ArduPlanePidConfig.h \
    ui/configuration/AdvParameterList.h \
    ui/configuration/ArduRoverPidConfig.h \
    ui/QGCConfigView.h \
    ui/main/QGCViewModeSelection.h \
    ui/main/QGCWelcomeMainWindow.h \
    ui/configuration/console.h \
    ui/configuration/SerialSettingsDialog.h \
    ui/configuration/terminalconsole.h \
    ui/configuration/ApmHighlighter.h \
    ui/configuration/ApmFirmwareConfig.h \
    uas/UASParameterDataModel.h \
    uas/UASParameterCommsMgr.h \
    ui/QGCPendingParamWidget.h \
    ui/px4_configuration/QGCPX4AirframeConfig.h \
    ui/QGCBaseParamWidget.h \
    ui/px4_configuration/QGCPX4MulticopterConfig.h \
    ui/px4_configuration/QGCPX4SensorCalibration.h \
    ui/designer/QGCXYPlot.h \
    ui/menuactionhelper.h \
    uas/UASManagerInterface.h \
    uas/QGCUASParamManagerInterface.h

SOURCES += \
    main.cc \
    QGCCore.cc \
    uas/UASManager.cc \
    uas/UAS.cc \
    comm/LinkManager.cc \
    comm/SerialLink.cc \
    comm/MAVLinkProtocol.cc \
    comm/QGCFlightGearLink.cc \
    comm/QGCJSBSimLink.cc \
    comm/QGCXPlaneLink.cc \
    ui/CommConfigurationWindow.cc \
    ui/SerialConfigurationWindow.cc \
    ui/MainWindow.cc \
    ui/uas/UASControlWidget.cc \
    ui/uas/UASListWidget.cc \
    ui/uas/UASInfoWidget.cc \
    ui/HUD.cc \
    ui/linechart/LinechartWidget.cc \
    ui/linechart/LinechartPlot.cc \
    ui/linechart/Scrollbar.cc \
    ui/linechart/ScrollZoomer.cc \
    ui/uas/UASView.cc \
    ui/CameraView.cc \
    comm/MAVLinkSimulationLink.cc \
    comm/UDPLink.cc \
    comm/TCPLink.cc \
    ui/ParameterInterface.cc \
    ui/WaypointList.cc \
    Waypoint.cc \
    ui/ObjectDetectionView.cc \
    input/JoystickInput.cc \
    ui/JoystickWidget.cc \
    ui/DebugConsole.cc \
    ui/HDDisplay.cc \
    ui/MAVLinkSettingsWidget.cc \
    ui/AudioOutputWidget.cc \
    GAudioOutput.cc \
    LogCompressor.cc \
    ui/QGCParamWidget.cc \
    ui/QGCSensorSettingsWidget.cc \
    ui/linechart/Linecharts.cc \
    uas/SlugsMAV.cc \
    uas/PxQuadMAV.cc \
    uas/ArduPilotMegaMAV.cc \
    uas/senseSoarMAV.cpp \
    ui/watchdog/WatchdogControl.cc \
    ui/watchdog/WatchdogProcessView.cc \
    ui/watchdog/WatchdogView.cc \
    uas/UASWaypointManager.cc \
    ui/HSIDisplay.cc \
    QGC.cc \
    ui/QGCFirmwareUpdate.cc \
    ui/QGCPxImuFirmwareUpdate.cc \
    ui/QGCDataPlot2D.cc \
    ui/linechart/IncrementalPlot.cc \
    ui/QGCRemoteControlView.cc \
    ui/RadioCalibration/RadioCalibrationWindow.cc \
    ui/RadioCalibration/AirfoilServoCalibrator.cc \
    ui/RadioCalibration/SwitchCalibrator.cc \
    ui/RadioCalibration/CurveCalibrator.cc \
    ui/RadioCalibration/AbstractCalibrator.cc \
    ui/RadioCalibration/RadioCalibrationData.cc \
    ui/QGCWebView.cc \
    ui/map3D/QGCWebPage.cc \
    ui/SlugsDataSensorView.cc \
    ui/SlugsHilSim.cc \
    ui/SlugsPadCameraControl.cpp \
    ui/QGCMainWindowAPConfigurator.cc \
    comm/MAVLinkSwarmSimulationLink.cc \
    ui/uas/QGCUnconnectedInfoWidget.cc \
    ui/designer/QGCToolWidget.cc \
    ui/designer/QGCParamSlider.cc \
    ui/designer/QGCCommandButton.cc \
    ui/designer/QGCToolWidgetItem.cc \
    ui/QGCMAVLinkLogPlayer.cc \
    comm/MAVLinkSimulationWaypointPlanner.cc \
    comm/MAVLinkSimulationMAV.cc \
    uas/QGCMAVLinkUASFactory.cc \
    ui/QGCWaypointListMulti.cc \
    ui/QGCUDPLinkConfiguration.cc \
    ui/QGCTCPLinkConfiguration.cc \
    ui/QGCSettingsWidget.cc \
    ui/uas/UASControlParameters.cpp \
    uas/QGCUASParamManager.cc \
    ui/map/QGCMapWidget.cc \
    ui/map/MAV2DIcon.cc \
    ui/map/Waypoint2DIcon.cc \
    ui/map/QGCMapTool.cc \
    ui/map/QGCMapToolBar.cc \
    ui/QGCToolBar.cc \
    ui/QGCStatusBar.cc \
    ui/QGCMAVLinkInspector.cc \
    ui/MAVLinkDecoder.cc \
    ui/WaypointViewOnlyView.cc \
    ui/WaypointEditableView.cc \
    ui/QGCRGBDView.cc \
    ui/mavlink/QGCMAVLinkMessageSender.cc \
    ui/firmwareupdate/QGCFirmwareUpdateWidget.cc \
    ui/QGCPluginHost.cc \
    ui/firmwareupdate/QGCPX4FirmwareUpdate.cc \
    ui/mission/QGCMissionOther.cc \
    ui/mission/QGCMissionNavWaypoint.cc \
    ui/mission/QGCMissionDoJump.cc \
    ui/mission/QGCMissionConditionDelay.cc \
    ui/mission/QGCMissionNavLoiterUnlim.cc \
    ui/mission/QGCMissionNavLoiterTurns.cc \
    ui/mission/QGCMissionNavLoiterTime.cc \
    ui/mission/QGCMissionNavReturnToLaunch.cc \
    ui/mission/QGCMissionNavLand.cc \
    ui/mission/QGCMissionNavTakeoff.cc \
    ui/mission/QGCMissionNavSweep.cc \
    ui/mission/QGCMissionDoStartSearch.cc \
    ui/mission/QGCMissionDoFinishSearch.cc \
    ui/QGCVehicleConfig.cc \
    ui/QGCPX4VehicleConfig.cc \
    ui/QGCHilConfiguration.cc \
    ui/QGCHilFlightGearConfiguration.cc \
    ui/QGCHilJSBSimConfiguration.cc \
    ui/QGCHilXPlaneConfiguration.cc \
    ui/designer/QGCComboBox.cc \
    ui/designer/QGCTextLabel.cc \
    ui/submainwindow.cpp \
    ui/uas/UASQuickViewItem.cc \
    ui/uas/UASQuickView.cc \
    ui/linechart/ChartPlot.cc \
    ui/uas/UASQuickViewTextItem.cc \
    ui/uas/UASQuickViewGaugeItem.cc \
    ui/uas/UASQuickViewItemSelect.cc \
    ui/uas/UASActionsWidget.cpp \
    ui/designer/QGCRadioChannelDisplay.cpp \
    ui/QGCTabbedInfoView.cpp \
    ui/UASRawStatusView.cpp \
    ui/PrimaryFlightDisplay.cc \
    ui/JoystickButton.cc \
    ui/JoystickAxis.cc \
    ui/uas/QGCMessageView.cc \
    ui/configuration/ApmHardwareConfig.cc \
    ui/configuration/ApmSoftwareConfig.cc \
    ui/configuration/FrameTypeConfig.cc \
    ui/configuration/CompassConfig.cc \
    ui/configuration/AccelCalibrationConfig.cc \
    ui/configuration/RadioCalibrationConfig.cc \
    ui/configuration/FlightModeConfig.cc \
    ui/configuration/Radio3DRConfig.cc \
    ui/configuration/BatteryMonitorConfig.cc \
    ui/configuration/SonarConfig.cc \
    ui/configuration/AirspeedConfig.cc \
    ui/configuration/OpticalFlowConfig.cc \
    ui/configuration/OsdConfig.cc \
    ui/configuration/AntennaTrackerConfig.cc \
    ui/configuration/CameraGimbalConfig.cc \
    ui/configuration/AP2ConfigWidget.cc \
    ui/configuration/BasicPidConfig.cc \
    ui/configuration/StandardParamConfig.cc \
    ui/configuration/GeoFenceConfig.cc \
    ui/configuration/FailSafeConfig.cc \
    ui/configuration/AdvancedParamConfig.cc \
    ui/configuration/ArduCopterPidConfig.cc \
    ui/apmtoolbar.cpp \
    ui/configuration/ApmPlaneLevel.cc \
    ui/configuration/ParamWidget.cc \
    ui/configuration/ArduPlanePidConfig.cc \
    ui/configuration/AdvParameterList.cc \
    ui/configuration/ArduRoverPidConfig.cc \
    ui/QGCConfigView.cc \
    ui/main/QGCViewModeSelection.cc \
    ui/main/QGCWelcomeMainWindow.cc \
    ui/configuration/terminalconsole.cpp \
    ui/configuration/console.cpp \
    ui/configuration/SerialSettingsDialog.cc \
    ui/configuration/ApmHighlighter.cc \
    ui/configuration/ApmFirmwareConfig.cc \
    uas/UASParameterDataModel.cc \
    uas/UASParameterCommsMgr.cc \
    ui/QGCPendingParamWidget.cc \
    ui/px4_configuration/QGCPX4AirframeConfig.cc \
    ui/QGCBaseParamWidget.cc \
    ui/px4_configuration/QGCPX4MulticopterConfig.cc \
    ui/px4_configuration/QGCPX4SensorCalibration.cc \
    ui/designer/QGCXYPlot.cc \
    ui/menuactionhelper.cpp
