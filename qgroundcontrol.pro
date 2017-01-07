# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2015 QGroundControl Developers
# License terms set in COPYING.md
# -------------------------------------------------

exists($${OUT_PWD}/qgroundcontrol.pro) {
    error("You must use shadow build (e.g. mkdir build; cd build; qmake ../qgroundcontrol.pro).")
}

message(Qt version $$[QT_VERSION])

!equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 4) {
    error("Unsupported Qt version, 5.5+ is required")
}

include(QGCCommon.pri)

TARGET   = QGroundControl
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

#
# Custom Build
#
# QGC will create a "CUSTOMCLASS" object (exposed by your custom build
# and derived from QGCCorePlugin).
# This is the start of allowing custom Plugins, which will eventually use a
# more defined runtime plugin architecture and not require a QGC project
# file you would have to keep in sync with the upstream repo.
#

# This allows you to ignore the custom build even if the custom build
# is present. It's useful to run "regular" builds to make sure you didn't
# break anything.

contains (CONFIG, QGC_DISABLE_CUSTOM_BUILD) {
    message("Disable custom build override")
} else {
    exists($$PWD/custom/custom.pri) {
        message("Found custom build")
        CONFIG  += CustomBuild
        DEFINES += QGC_CUSTOM_BUILD
        # custom.pri must define:
        # CUSTOMCLASS  = YourIQGCCorePluginDerivation
        # CUSTOMHEADER = \"\\\"YourIQGCCorePluginDerivation.h\\\"\"
        include($$PWD/custom/custom.pri)
    }
}

#
# Plugin configuration
#
# This allows you to build custom versions of QGC which only includes your
# specific vehicle plugin. To remove support for a firmware type completely,
# disable both the Plugin and PluginFactory entries. To include custom support
# for an existing plugin type disable PluginFactory only. Then provide you own
# implementation of FirmwarePluginFactory and use the FirmwarePlugin and
# AutoPilotPlugin classes as the base clase for your derived plugin
# implementation.

contains (CONFIG, QGC_DISABLE_APM_PLUGIN) {
    message("Disable APM Plugin")
} else {
    CONFIG += APMFirmwarePlugin
}

contains (CONFIG, QGC_DISABLE_APM_PLUGIN_FACTORY) {
    message("Disable APM Plugin Factory")
} else {
    CONFIG += APMFirmwarePluginFactory
}

contains (CONFIG, QGC_DISABLE_PX4_PLUGIN) {
    message("Disable PX4 Plugin")
} else {
    CONFIG += PX4FirmwarePlugin
}

contains (CONFIG, QGC_DISABLE_PX4_PLUGIN_FACTORY) {
    message("Disable PX4 Plugin Factory")
} else {
    CONFIG += PX4FirmwarePluginFactory
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

# USB Camera and UVC Video Sources
contains (DEFINES, QGC_DISABLE_UVC) {
    message("Skipping support for UVC devices (manual override from command line)")
    DEFINES += QGC_DISABLE_UVC
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_DISABLE_UVC) {
    message("Skipping support for UVC devices (manual override from user_config.pri)")
    DEFINES += QGC_DISABLE_UVC
} else:LinuxBuild {
    contains(QT_VERSION, 5.5.1) {
        message("Skipping support for UVC devices (conflict with Qt 5.5.1 on Ubuntu)")
        DEFINES += QGC_DISABLE_UVC
    }
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
    xml

# Multimedia only used if QVC is enabled
!contains (DEFINES, QGC_DISABLE_UVC) {
    QT += \
        multimedia
}

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
        #-- Create official, versioned Info.plist
        APP_STORE = $$system(cd $${BASEDIR} && $${BASEDIR}/tools/update_ios_version.sh $${BASEDIR}/ios/iOSForAppStore-Info-Source.plist $${BASEDIR}/ios/iOSForAppStore-Info.plist)
        APP_ERROR = $$find(APP_STORE, "Error")
        count(APP_ERROR, 1) {
            error("Error building .plist file. 'ForAppStore' builds are only possible through the official build system.")
        }
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
QMAKE_TARGET_PRODUCT = "QGroundControl"

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

DebugBuild {
    # Unit Test resources
    RESOURCES += UnitTest.qrc
}

DEPENDPATH += \
    . \
    plugins

INCLUDEPATH += .

INCLUDEPATH += \
    include/ui \
    src \
    src/api \
    src/AnalyzeView \
    src/AutoPilotPlugins \
    src/FlightDisplay \
    src/FlightMap \
    src/FlightMap/Widgets \
    src/FollowMe \
    src/GPS \
    src/Joystick \
    src/MissionEditor \
    src/MissionManager \
    src/PositionManager \
    src/QmlControls \
    src/QtLocationPlugin \
    src/QtLocationPlugin/QMLControl \
    src/VehicleSetup \
    src/ViewWidgets \
    src/audio \
    src/comm \
    src/input \
    src/lib/qmapcontrol \
    src/uas \
    src/ui \
    src/ui/linechart \
    src/ui/map \
    src/ui/mapdisplay \
    src/ui/mission \
    src/ui/px4_configuration \
    src/ui/toolbar \
    src/ui/uas \

FORMS += \
    src/ui/MainWindow.ui \
    src/QGCQmlWidgetHolder.ui \

!MobileBuild {
FORMS += \
    src/ui/Linechart.ui \
    src/ui/MultiVehicleDockWidget.ui \
    src/ui/QGCHilConfiguration.ui \
    src/ui/QGCHilFlightGearConfiguration.ui \
    src/ui/QGCHilJSBSimConfiguration.ui \
    src/ui/QGCHilXPlaneConfiguration.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/QGCMAVLinkLogPlayer.ui \
    src/ui/QGCMapRCToParamDialog.ui \
    src/ui/QGCTabbedInfoView.ui \
    src/ui/QGCUASFileView.ui \
    src/ui/QGCUASFileViewMulti.ui \
    src/ui/uas/QGCUnconnectedInfoWidget.ui \
    src/ui/uas/UASMessageView.ui \
    src/ui/uas/UASQuickView.ui \
    src/ui/uas/UASQuickViewItemSelect.ui \
}

#
# Plugin API
#

HEADERS += \
    src/api/QGCCorePlugin.h \
    src/api/QGCOptions.h \
    src/api/QGCSettings.h \

SOURCES += \
    src/api/QGCCorePlugin.cc \
    src/api/QGCOptions.cc \
    src/api/QGCSettings.cc \

#
# Unit Test specific configuration goes here (requires full debug build with all plugins)
#

DebugBuild { PX4FirmwarePlugin { PX4FirmwarePluginFactory  { APMFirmwarePlugin { APMFirmwarePluginFactory { !MobileBuild {
    DEFINES += UNITTEST_BUILD

    INCLUDEPATH += \
        src/qgcunittest

    HEADERS += \
        src/AnalyzeView/LogDownloadTest.h \
        src/FactSystem/FactSystemTestBase.h \
        src/FactSystem/FactSystemTestGeneric.h \
        src/FactSystem/FactSystemTestPX4.h \
        src/FactSystem/ParameterManagerTest.h \
        src/MissionManager/ComplexMissionItemTest.h \
        src/MissionManager/MissionCommandTreeTest.h \
        src/MissionManager/MissionControllerManagerTest.h \
        src/MissionManager/MissionControllerTest.h \
        src/MissionManager/MissionItemTest.h \
        src/MissionManager/MissionManagerTest.h \
        src/MissionManager/SimpleMissionItemTest.h \
        src/qgcunittest/FileDialogTest.h \
        src/qgcunittest/FileManagerTest.h \
        src/qgcunittest/FlightGearTest.h \
        src/qgcunittest/GeoTest.h \
        src/qgcunittest/LinkManagerTest.h \
        src/qgcunittest/MainWindowTest.h \
        src/qgcunittest/MavlinkLogTest.h \
        src/qgcunittest/MessageBoxTest.h \
        src/qgcunittest/MultiSignalSpy.h \
        src/qgcunittest/RadioConfigTest.h \
        src/qgcunittest/TCPLinkTest.h \
        src/qgcunittest/TCPLoopBackServer.h \
        src/qgcunittest/UnitTest.h \
        src/Vehicle/SendMavCommandTest.h \

    SOURCES += \
        src/AnalyzeView/LogDownloadTest.cc \
        src/FactSystem/FactSystemTestBase.cc \
        src/FactSystem/FactSystemTestGeneric.cc \
        src/FactSystem/FactSystemTestPX4.cc \
        src/FactSystem/ParameterManagerTest.cc \
        src/MissionManager/ComplexMissionItemTest.cc \
        src/MissionManager/MissionCommandTreeTest.cc \
        src/MissionManager/MissionControllerManagerTest.cc \
        src/MissionManager/MissionControllerTest.cc \
        src/MissionManager/MissionItemTest.cc \
        src/MissionManager/MissionManagerTest.cc \
        src/MissionManager/SimpleMissionItemTest.cc \
        src/qgcunittest/FileDialogTest.cc \
        src/qgcunittest/FileManagerTest.cc \
        src/qgcunittest/FlightGearTest.cc \
        src/qgcunittest/GeoTest.cc \
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
        src/Vehicle/SendMavCommandTest.cc \
} } } } } }

# Main QGC Headers and Source files

HEADERS += \
    src/AnalyzeView/ExifParser.h \
    src/CmdLineOptParser.h \
    src/FirmwarePlugin/PX4/px4_custom_mode.h \
    src/FlightDisplay/VideoManager.h \
    src/FlightMap/FlightMapSettings.h \
    src/FlightMap/Widgets/ValuesWidgetController.h \
    src/FollowMe/FollowMe.h \
    src/GAudioOutput.h \
    src/Joystick/Joystick.h \
    src/Joystick/JoystickManager.h \
    src/JsonHelper.h \
    src/LogCompressor.h \
    src/MG.h \
    src/MissionManager/ComplexMissionItem.h \
    src/MissionManager/GeoFenceController.h \
    src/MissionManager/GeoFenceManager.h \
    src/MissionManager/MissionCommandList.h \
    src/MissionManager/MissionCommandTree.h \
    src/MissionManager/MissionCommandUIInfo.h \
    src/MissionManager/MissionController.h \
    src/MissionManager/MissionItem.h \
    src/MissionManager/MissionManager.h \
    src/MissionManager/PlanElementController.h \
    src/MissionManager/QGCMapPolygon.h \
    src/MissionManager/RallyPoint.h \
    src/MissionManager/RallyPointController.h \
    src/MissionManager/RallyPointManager.h \
    src/MissionManager/SimpleMissionItem.h \
    src/MissionManager/SurveyMissionItem.h \
    src/MissionManager/VisualMissionItem.h \
    src/PositionManager/PositionManager.h \
    src/PositionManager/SimulatedPosition.h \
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
    src/QmlControls/QGCImageProvider.h \
    src/QmlControls/QGroundControlQmlGlobal.h \
    src/QmlControls/QmlObjectListModel.h \
    src/QmlControls/RCChannelMonitorController.h \
    src/QmlControls/ScreenToolsController.h \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.h \
    src/Vehicle/MAVLinkLogManager.h \
    src/VehicleSetup/JoystickConfigController.h \
    src/audio/QGCAudioWorker.h \
    src/comm/LinkConfiguration.h \
    src/comm/LinkInterface.h \
    src/comm/LinkManager.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/ProtocolInterface.h \
    src/comm/QGCMAVLink.h \
    src/comm/TCPLink.h \
    src/comm/UDPLink.h \
    src/uas/UAS.h \
    src/uas/UASInterface.h \
    src/uas/UASMessageHandler.h \
    src/ui/toolbar/MainToolBarController.h \

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

!NoSerialBuild {
HEADERS += \
    src/comm/QGCSerialPortInfo.h \
    src/comm/SerialLink.h \
}

!MobileBuild {
HEADERS += \
    src/AnalyzeView/GeoTagController.h \
    src/AnalyzeView/LogDownloadController.h \
    src/GPS/Drivers/src/gps_helper.h \
    src/GPS/Drivers/src/ubx.h \
    src/GPS/GPSManager.h \
    src/GPS/GPSPositionMessage.h \
    src/GPS/GPSProvider.h \
    src/GPS/RTCM/RTCMMavlink.h \
    src/GPS/definitions.h \
    src/GPS/satellite_info.h \
    src/GPS/vehicle_gps_position.h \
    src/Joystick/JoystickSDL.h \
    src/QGCFileDialog.h \
    src/QGCMessageBox.h \
    src/RunGuard.h \
    src/ViewWidgets/CustomCommandWidget.h \
    src/ViewWidgets/CustomCommandWidgetController.h \
    src/ViewWidgets/ViewWidgetController.h \
    src/comm/LogReplayLink.h \
    src/comm/QGCFlightGearLink.h \
    src/comm/QGCHilLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
    src/uas/FileManager.h \
    src/ui/HILDockWidget.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/MainWindow.h \
    src/ui/MultiVehicleDockWidget.h \
    src/ui/QGCHilConfiguration.h \
    src/ui/QGCHilFlightGearConfiguration.h \
    src/ui/QGCHilJSBSimConfiguration.h \
    src/ui/QGCHilXPlaneConfiguration.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/QGCMAVLinkLogPlayer.h \
    src/ui/QGCMapRCToParamDialog.h \
    src/ui/QGCTabbedInfoView.h \
    src/ui/QGCUASFileView.h \
    src/ui/QGCUASFileViewMulti.h \
    src/ui/linechart/ChartPlot.h \
    src/ui/linechart/IncrementalPlot.h \
    src/ui/linechart/LinechartPlot.h \
    src/ui/linechart/LinechartWidget.h \
    src/ui/linechart/Linecharts.h \
    src/ui/linechart/ScrollZoomer.h \
    src/ui/linechart/Scrollbar.h \
    src/ui/uas/QGCUnconnectedInfoWidget.h \
    src/ui/uas/UASMessageView.h \
    src/ui/uas/UASQuickView.h \
    src/ui/uas/UASQuickViewGaugeItem.h \
    src/ui/uas/UASQuickViewItem.h \
    src/ui/uas/UASQuickViewItemSelect.h \
    src/ui/uas/UASQuickViewTextItem.h \
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
    src/AnalyzeView/ExifParser.cc \
    src/CmdLineOptParser.cc \
    src/FlightDisplay/VideoManager.cc \
    src/FlightMap/FlightMapSettings.cc \
    src/FlightMap/Widgets/ValuesWidgetController.cc \
    src/FollowMe/FollowMe.cc \
    src/GAudioOutput.cc \
    src/Joystick/Joystick.cc \
    src/Joystick/JoystickManager.cc \
    src/JsonHelper.cc \
    src/LogCompressor.cc \
    src/MissionManager/ComplexMissionItem.cc \
    src/MissionManager/GeoFenceController.cc \
    src/MissionManager/GeoFenceManager.cc \
    src/MissionManager/MissionCommandList.cc \
    src/MissionManager/MissionCommandTree.cc \
    src/MissionManager/MissionCommandUIInfo.cc \
    src/MissionManager/MissionController.cc \
    src/MissionManager/MissionItem.cc \
    src/MissionManager/MissionManager.cc \
    src/MissionManager/PlanElementController.cc \
    src/MissionManager/QGCMapPolygon.cc \
    src/MissionManager/RallyPoint.cc \
    src/MissionManager/RallyPointController.cc \
    src/MissionManager/RallyPointManager.cc \
    src/MissionManager/SimpleMissionItem.cc \
    src/MissionManager/SurveyMissionItem.cc \
    src/MissionManager/VisualMissionItem.cc \
    src/PositionManager/PositionManager.cpp \
    src/PositionManager/SimulatedPosition.cc \
    src/QGC.cc \
    src/QGCApplication.cc \
    src/QGCComboBox.cc \
    src/QGCDockWidget.cc \
    src/QGCFileDownload.cc \
    src/QGCGeo.cc \
    src/QGCLoggingCategory.cc \
    src/QGCMapPalette.cc \
    src/QGCMobileFileDialogController.cc \
    src/QGCPalette.cc \
    src/QGCQmlWidgetHolder.cpp \
    src/QGCQuickWidget.cc \
    src/QGCTemporaryFile.cc \
    src/QGCToolbox.cc \
    src/QmlControls/AppMessages.cc \
    src/QmlControls/CoordinateVector.cc \
    src/QmlControls/ParameterEditorController.cc \
    src/QmlControls/QGCImageProvider.cc \
    src/QmlControls/QGroundControlQmlGlobal.cc \
    src/QmlControls/QmlObjectListModel.cc \
    src/QmlControls/RCChannelMonitorController.cc \
    src/QmlControls/ScreenToolsController.cc \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.cc \
    src/Vehicle/MAVLinkLogManager.cc \
    src/VehicleSetup/JoystickConfigController.cc \
    src/audio/QGCAudioWorker.cpp \
    src/comm/LinkConfiguration.cc \
    src/comm/LinkInterface.cc \
    src/comm/LinkManager.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/QGCMAVLink.cc \
    src/comm/TCPLink.cc \
    src/comm/UDPLink.cc \
    src/main.cc \
    src/uas/UAS.cc \
    src/uas/UASMessageHandler.cc \
    src/ui/toolbar/MainToolBarController.cc \

DebugBuild {
SOURCES += \
    src/comm/MockLink.cc \
    src/comm/MockLinkFileServer.cc \
    src/comm/MockLinkMissionItemHandler.cc \
}

!NoSerialBuild {
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
    src/AnalyzeView/GeoTagController.cc \
    src/AnalyzeView/LogDownloadController.cc \
    src/GPS/Drivers/src/gps_helper.cpp \
    src/GPS/Drivers/src/ubx.cpp \
    src/GPS/GPSManager.cc \
    src/GPS/GPSProvider.cc \
    src/GPS/RTCM/RTCMMavlink.cc \
    src/Joystick/JoystickSDL.cc \
    src/QGCFileDialog.cc \
    src/RunGuard.cc \
    src/ViewWidgets/CustomCommandWidget.cc \
    src/ViewWidgets/CustomCommandWidgetController.cc \
    src/ViewWidgets/ViewWidgetController.cc \
    src/comm/LogReplayLink.cc \
    src/comm/QGCFlightGearLink.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
    src/uas/FileManager.cc \
    src/ui/HILDockWidget.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/MainWindow.cc \
    src/ui/MultiVehicleDockWidget.cc \
    src/ui/QGCHilConfiguration.cc \
    src/ui/QGCHilFlightGearConfiguration.cc \
    src/ui/QGCHilJSBSimConfiguration.cc \
    src/ui/QGCHilXPlaneConfiguration.cc \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/QGCMAVLinkLogPlayer.cc \
    src/ui/QGCMapRCToParamDialog.cpp \
    src/ui/QGCTabbedInfoView.cpp \
    src/ui/QGCUASFileView.cc \
    src/ui/QGCUASFileViewMulti.cc \
    src/ui/linechart/ChartPlot.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/linechart/LinechartPlot.cc \
    src/ui/linechart/LinechartWidget.cc \
    src/ui/linechart/Linecharts.cc \
    src/ui/linechart/ScrollZoomer.cc \
    src/ui/linechart/Scrollbar.cc \
    src/ui/uas/QGCUnconnectedInfoWidget.cc \
    src/ui/uas/UASMessageView.cc \
    src/ui/uas/UASQuickView.cc \
    src/ui/uas/UASQuickViewGaugeItem.cc \
    src/ui/uas/UASQuickViewItem.cc \
    src/ui/uas/UASQuickViewItemSelect.cc \
    src/ui/uas/UASQuickViewTextItem.cc \
}

# Palette test widget in debug builds
DebugBuild {
    HEADERS += src/QmlControls/QmlTestWidget.h
    SOURCES += src/QmlControls/QmlTestWidget.cc
}

#
# Firmware Plugin Support
#

INCLUDEPATH += \
    src/AutoPilotPlugins/Common \
    src/FirmwarePlugin \
    src/Vehicle \
    src/VehicleSetup \

HEADERS+= \
    src/AutoPilotPlugins/AutoPilotPlugin.h \
    src/AutoPilotPlugins/Common/ESP8266Component.h \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.h \
    src/AutoPilotPlugins/Common/MotorComponent.h \
    src/AutoPilotPlugins/Common/RadioComponentController.h \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.h \
    src/FirmwarePlugin/FirmwarePlugin.h \
    src/FirmwarePlugin/FirmwarePluginManager.h \
    src/Vehicle/MultiVehicleManager.h \
    src/Vehicle/Vehicle.h \
    src/VehicleSetup/VehicleComponent.h \

!MobileBuild {
    HEADERS += \
        src/VehicleSetup/Bootloader.h \
        src/VehicleSetup/FirmwareImage.h \
        src/VehicleSetup/FirmwareUpgradeController.h \
        src/VehicleSetup/PX4FirmwareUpgradeThread.h \
}

SOURCES += \
    src/AutoPilotPlugins/AutoPilotPlugin.cc \
    src/AutoPilotPlugins/Common/ESP8266Component.cc \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.cc \
    src/AutoPilotPlugins/Common/MotorComponent.cc \
    src/AutoPilotPlugins/Common/RadioComponentController.cc \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc \
    src/FirmwarePlugin/FirmwarePlugin.cc \
    src/FirmwarePlugin/FirmwarePluginManager.cc \
    src/Vehicle/MultiVehicleManager.cc \
    src/Vehicle/Vehicle.cc \
    src/VehicleSetup/VehicleComponent.cc \

!MobileBuild {
    SOURCES += \
        src/VehicleSetup/Bootloader.cc \
        src/VehicleSetup/FirmwareImage.cc \
        src/VehicleSetup/FirmwareUpgradeController.cc \
        src/VehicleSetup/PX4FirmwareUpgradeThread.cc \
}

# ArduPilot FirmwarePlugin

APMFirmwarePlugin {
    RESOURCES *= src/FirmwarePlugin/APM/APMResources.qrc

    INCLUDEPATH += \
        src/AutoPilotPlugins/APM \
        src/FirmwarePlugin/APM \

    HEADERS += \
        src/AutoPilotPlugins/APM/APMAirframeComponent.h \
        src/AutoPilotPlugins/APM/APMAirframeComponentAirframes.h \
        src/AutoPilotPlugins/APM/APMAirframeComponentController.h \
        src/AutoPilotPlugins/APM/APMAirframeLoader.h \
        src/AutoPilotPlugins/APM/APMAutoPilotPlugin.h \
        src/AutoPilotPlugins/APM/APMCameraComponent.h \
        src/AutoPilotPlugins/APM/APMCompassCal.h \
        src/AutoPilotPlugins/APM/APMFlightModesComponent.h \
        src/AutoPilotPlugins/APM/APMFlightModesComponentController.h \
        src/AutoPilotPlugins/APM/APMLightsComponent.h \
        src/AutoPilotPlugins/APM/APMSubFrameComponent.h \
        src/AutoPilotPlugins/APM/APMPowerComponent.h \
        src/AutoPilotPlugins/APM/APMRadioComponent.h \
        src/AutoPilotPlugins/APM/APMSafetyComponent.h \
        src/AutoPilotPlugins/APM/APMSensorsComponent.h \
        src/AutoPilotPlugins/APM/APMSensorsComponentController.h \
        src/AutoPilotPlugins/APM/APMTuningComponent.h \
        src/FirmwarePlugin/APM/APMFirmwarePlugin.h \
        src/FirmwarePlugin/APM/APMGeoFenceManager.h \
        src/FirmwarePlugin/APM/APMParameterMetaData.h \
        src/FirmwarePlugin/APM/APMRallyPointManager.h \
        src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduSubFirmwarePlugin.h \

    SOURCES += \
        src/AutoPilotPlugins/APM/APMAirframeComponent.cc \
        src/AutoPilotPlugins/APM/APMAirframeComponentAirframes.cc \
        src/AutoPilotPlugins/APM/APMAirframeComponentController.cc \
        src/AutoPilotPlugins/APM/APMAirframeLoader.cc \
        src/AutoPilotPlugins/APM/APMAutoPilotPlugin.cc \
        src/AutoPilotPlugins/APM/APMCameraComponent.cc \
        src/AutoPilotPlugins/APM/APMCompassCal.cc \
        src/AutoPilotPlugins/APM/APMFlightModesComponent.cc \
        src/AutoPilotPlugins/APM/APMFlightModesComponentController.cc \
        src/AutoPilotPlugins/APM/APMLightsComponent.cc \
        src/AutoPilotPlugins/APM/APMSubFrameComponent.cc \
        src/AutoPilotPlugins/APM/APMPowerComponent.cc \
        src/AutoPilotPlugins/APM/APMRadioComponent.cc \
        src/AutoPilotPlugins/APM/APMSafetyComponent.cc \
        src/AutoPilotPlugins/APM/APMSensorsComponent.cc \
        src/AutoPilotPlugins/APM/APMSensorsComponentController.cc \
        src/AutoPilotPlugins/APM/APMTuningComponent.cc \
        src/FirmwarePlugin/APM/APMFirmwarePlugin.cc \
        src/FirmwarePlugin/APM/APMGeoFenceManager.cc \
        src/FirmwarePlugin/APM/APMParameterMetaData.cc \
        src/FirmwarePlugin/APM/APMRallyPointManager.cc \
        src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.cc \
        src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc \
        src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc \
        src/FirmwarePlugin/APM/ArduSubFirmwarePlugin.cc \
}

APMFirmwarePluginFactory {
    HEADERS   += src/FirmwarePlugin/APM/APMFirmwarePluginFactory.h
    SOURCES   += src/FirmwarePlugin/APM/APMFirmwarePluginFactory.cc
}

# PX4 FirmwarePlugin

PX4FirmwarePlugin {
    RESOURCES *= src/FirmwarePlugin/PX4/PX4Resources.qrc

    INCLUDEPATH += \
        src/AutoPilotPlugins/PX4 \
        src/FirmwarePlugin/PX4 \

    HEADERS+= \
        src/AutoPilotPlugins/PX4/AirframeComponent.h \
        src/AutoPilotPlugins/PX4/AirframeComponentAirframes.h \
        src/AutoPilotPlugins/PX4/AirframeComponentController.h \
        src/AutoPilotPlugins/PX4/CameraComponent.h \
        src/AutoPilotPlugins/PX4/FlightModesComponent.h \
        src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.h \
        src/AutoPilotPlugins/PX4/PX4AirframeLoader.h \
        src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h \
        src/AutoPilotPlugins/PX4/PX4RadioComponent.h \
        src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.h \
        src/AutoPilotPlugins/PX4/PX4TuningComponent.h \
        src/AutoPilotPlugins/PX4/PowerComponent.h \
        src/AutoPilotPlugins/PX4/PowerComponentController.h \
        src/AutoPilotPlugins/PX4/SafetyComponent.h \
        src/AutoPilotPlugins/PX4/SensorsComponent.h \
        src/AutoPilotPlugins/PX4/SensorsComponentController.h \
        src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h \
        src/FirmwarePlugin/PX4/PX4GeoFenceManager.h \
        src/FirmwarePlugin/PX4/PX4ParameterMetaData.h \

    SOURCES += \
        src/AutoPilotPlugins/PX4/AirframeComponent.cc \
        src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc \
        src/AutoPilotPlugins/PX4/AirframeComponentController.cc \
        src/AutoPilotPlugins/PX4/CameraComponent.cc \
        src/AutoPilotPlugins/PX4/FlightModesComponent.cc \
        src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.cc \
        src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc \
        src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc \
        src/AutoPilotPlugins/PX4/PX4RadioComponent.cc \
        src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.cc \
        src/AutoPilotPlugins/PX4/PX4TuningComponent.cc \
        src/AutoPilotPlugins/PX4/PowerComponent.cc \
        src/AutoPilotPlugins/PX4/PowerComponentController.cc \
        src/AutoPilotPlugins/PX4/SafetyComponent.cc \
        src/AutoPilotPlugins/PX4/SensorsComponent.cc \
        src/AutoPilotPlugins/PX4/SensorsComponentController.cc \
        src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc \
        src/FirmwarePlugin/PX4/PX4GeoFenceManager.cc \
        src/FirmwarePlugin/PX4/PX4ParameterMetaData.cc \
}

PX4FirmwarePluginFactory {
    HEADERS   += src/FirmwarePlugin/PX4/PX4FirmwarePluginFactory.h
    SOURCES   += src/FirmwarePlugin/PX4/PX4FirmwarePluginFactory.cc
}

# Fact System code

INCLUDEPATH += \
    src/FactSystem \
    src/FactSystem/FactControls \

HEADERS += \
    src/FactSystem/Fact.h \
    src/FactSystem/FactControls/FactPanelController.h \
    src/FactSystem/FactGroup.h \
    src/FactSystem/FactMetaData.h \
    src/FactSystem/FactSystem.h \
    src/FactSystem/FactValidator.h \
    src/FactSystem/ParameterManager.h \
    src/FactSystem/SettingsFact.h \

SOURCES += \
    src/FactSystem/Fact.cc \
    src/FactSystem/FactControls/FactPanelController.cc \
    src/FactSystem/FactGroup.cc \
    src/FactSystem/FactMetaData.cc \
    src/FactSystem/FactSystem.cc \
    src/FactSystem/FactValidator.cc \
    src/FactSystem/ParameterManager.cc \
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

contains (CONFIG, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from command line)")
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from user_config.pri)")
} else {
    include(src/VideoStreaming/VideoStreaming.pri)
}

#-------------------------------------------------------------------------------------
# Android

AndroidBuild {
    contains (CONFIG, DISABLE_BUILTIN_ANDROID) {
        message("Skipping builtin support for Android")
    } else {
        include(android.pri)
    }
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
