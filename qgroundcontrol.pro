# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2019 QGroundControl Developers
# License terms set in COPYING.md
# -------------------------------------------------

QMAKE_PROJECT_DEPTH = 0 # undocumented qmake flag to force absolute paths in make files

exists($${OUT_PWD}/qgroundcontrol.pro) {
    error("You must use shadow build (e.g. mkdir build; cd build; qmake ../qgroundcontrol.pro).")
}

message(Qt version $$[QT_VERSION])

!equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 10) {
    error("Unsupported Qt version, 5.11+ is required")
}

include(QGCCommon.pri)

TARGET   = QGroundControl
TEMPLATE = app
QGCROOT  = $$PWD

DebugBuild {
    DESTDIR  = $${OUT_PWD}/debug
} else {
    DESTDIR  = $${OUT_PWD}/release
}

QML_IMPORT_PATH += $$PWD/src/QmlControls

#
# OS Specific settings
#

MacBuild {
    QMAKE_INFO_PLIST    = Custom-Info.plist
    ICON                = $${BASEDIR}/resources/icons/macx.icns
    OTHER_FILES        += Custom-Info.plist
    LIBS               += -framework ApplicationServices
}

LinuxBuild {
    CONFIG  += qesp_linux_udev
}

WindowsBuild {
    RC_ICONS = resources/icons/qgroundcontrol.ico
    CONFIG += resources_big
}

#
# Branding
#

QGC_APP_NAME        = "QGroundControl"
QGC_ORG_NAME        = "QGroundControl.org"
QGC_ORG_DOMAIN      = "org.qgroundcontrol"
QGC_APP_DESCRIPTION = "Open source ground control app provided by QGroundControl dev team"
QGC_APP_COPYRIGHT   = "Copyright (C) 2019 QGroundControl Development Team. All rights reserved."

WindowsBuild {
    QGC_INSTALLER_ICON          = "WindowsQGC.ico"
    QGC_INSTALLER_HEADER_BITMAP = "installheader.bmp"
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

WindowsBuild {
    # Sets up application properties
    QMAKE_TARGET_COMPANY        = "$${QGC_ORG_NAME}"
    QMAKE_TARGET_DESCRIPTION    = "$${QGC_APP_DESCRIPTION}"
    QMAKE_TARGET_COPYRIGHT      = "$${QGC_APP_COPYRIGHT}"
    QMAKE_TARGET_PRODUCT        = "$${QGC_APP_NAME}"
}

#-------------------------------------------------------------------------------------
# iOS

iOSBuild {
    contains (CONFIG, DISABLE_BUILTIN_IOS) {
        message("Skipping builtin support for iOS")
    } else {
        LIBS                 += -framework AVFoundation
        #-- Info.plist (need an "official" one for the App Store)
        ForAppStore {
            message(App Store Build)
            #-- Create official, versioned Info.plist
            APP_STORE = $$system(cd $${BASEDIR} && $${BASEDIR}/tools/update_ios_version.sh $${BASEDIR}/ios/iOSForAppStore-Info-Source.plist $${BASEDIR}/ios/iOSForAppStore-Info.plist)
            APP_ERROR = $$find(APP_STORE, "Error")
            count(APP_ERROR, 1) {
                error("Error building .plist file. 'ForAppStore' builds are only possible through the official build system.")
            }
            QT               += qml-private
            QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOSForAppStore-Info.plist
            OTHER_FILES      += $${BASEDIR}/ios/iOSForAppStore-Info.plist
        } else {
            QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOS-Info.plist
            OTHER_FILES      += $${BASEDIR}/ios/iOS-Info.plist
        }
        QMAKE_ASSET_CATALOGS += ios/Images.xcassets
        BUNDLE.files          = ios/QGCLaunchScreen.xib $$QMAKE_INFO_PLIST
        QMAKE_BUNDLE_DATA    += BUNDLE
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

# QTNFC
contains (DEFINES, QGC_DISABLE_QTNFC) {
    message("Skipping support for QTNFC (manual override from command line)")
    DEFINES -= QGC_ENABLE_QTNFC
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_DISABLE_QTNFC) {
    message("Skipping support for QTNFC (manual override from user_config.pri)")
    DEFINES -= QGC_ENABLE_QTNFC
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_ENABLE_QTNFC) {
    message("Including support for QTNFC (manual override from user_config.pri)")
    DEFINES += QGC_ENABLE_QTNFC
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
    c++11

DebugBuild {
    CONFIG -= qtquickcompiler
} else {
    CONFIG += qtquickcompiler
}

contains(DEFINES, ENABLE_VERBOSE_OUTPUT) {
    message("Enable verbose compiler output (manual override from command line)")
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, ENABLE_VERBOSE_OUTPUT) {
    message("Enable verbose compiler output (manual override from user_config.pri)")
} else {
    CONFIG += silent
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
    texttospeech

# Multimedia only used if QVC is enabled
!contains (DEFINES, QGC_DISABLE_UVC) {
    QT += \
        multimedia
}

AndroidBuild || iOSBuild {
    # Android and iOS don't unclude these
} else {
    QT += \
        printsupport \
        serialport \
}

contains(DEFINES, QGC_ENABLE_BLUETOOTH) {
QT += \
    bluetooth \
}

contains(DEFINES, QGC_ENABLE_QTNFC) {
QT += \
    nfc \
}

#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib
ReleaseBuild {
    # We don't need the testlib console in release mode
    QT.testlib.CONFIG -= console
}

#
# Build-specific settings
#

DebugBuild {
!iOSBuild {
    CONFIG += console
}
}

#
# Our QtLocation "plugin"
#

include(src/QtLocationPlugin/QGCLocationPlugin.pri)

# Until pairing can be made to work cleanly on all OS it is turned off
DEFINES+=QGC_DISABLE_PAIRING

# Pairing
contains (DEFINES, QGC_DISABLE_PAIRING) {
    message("Skipping support for Pairing")
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, QGC_DISABLE_PAIRING) {
    message("Skipping support for Pairing (manual override from user_config.pri)")
} else:AndroidBuild:contains(QT_ARCH, arm64) {
    # Haven't figured out how to get 64 bit arm OpenSLL yet which pairing requires
    message("Skipping support for Pairing (Missing Android OpenSSL 64 bit support)")
} else {
    message("Enabling support for Pairing")
    DEFINES += QGC_ENABLE_PAIRING
}

#
# External library configuration
#

include(QGCExternalLibs.pri)

#
# Resources (custom code can replace them)
#

CustomBuild {
    exists($$PWD/custom/qgroundcontrol.qrc) {
        message("Using custom qgroundcontrol.qrc")
        RESOURCES += $$PWD/custom/qgroundcontrol.qrc
    } else {
        RESOURCES += $$PWD/qgroundcontrol.qrc
    }
    exists($$PWD/custom/qgcresources.qrc) {
        message("Using custom qgcresources.qrc")
        RESOURCES += $$PWD/custom/qgcresources.qrc
    } else {
        RESOURCES += $$PWD/qgcresources.qrc
    }
    exists($$PWD/custom/qgcimages.qrc) {
        message("Using custom qgcimages.qrc")
        RESOURCES += $$PWD/custom/qgcimages.qrc
    } else {
        RESOURCES += $$PWD/qgcimages.qrc
    }
} else {
    DEFINES += QGC_APPLICATION_NAME=\"\\\"QGroundControl\\\"\"
    DEFINES += QGC_ORG_NAME=\"\\\"QGroundControl.org\\\"\"
    DEFINES += QGC_ORG_DOMAIN=\"\\\"org.qgroundcontrol\\\"\"
    RESOURCES += \
        $$PWD/qgroundcontrol.qrc \
        $$PWD/qgcresources.qrc \
        $$PWD/qgcimages.qrc
}

# On Qt 5.9 android versions there is the following bug: https://bugreports.qt.io/browse/QTBUG-61424
# This prevents FileDialog from being used. So we have a temp hack workaround for it which just no-ops
# the FileDialog fallback mechanism on android 5.9 builds.
equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 9):AndroidBuild {
    RESOURCES += $$PWD/HackAndroidFileDialog.qrc
} else {
    RESOURCES += $$PWD/HackFileDialog.qrc
}

#
# Main QGroundControl portion of project file
#

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
    src/ADSB \
    src/api \
    src/AnalyzeView \
    src/Camera \
    src/AutoPilotPlugins \
    src/FlightDisplay \
    src/FlightMap \
    src/FlightMap/Widgets \
    src/FollowMe \
    src/Geo \
    src/GPS \
    src/Joystick \
    src/PlanView \
    src/MissionManager \
    src/PositionManager \
    src/QmlControls \
    src/QtLocationPlugin \
    src/QtLocationPlugin/QMLControl \
    src/Settings \
    src/Terrain \
    src/Vehicle \
    src/ViewWidgets \
    src/Audio \
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

contains (DEFINES, QGC_ENABLE_PAIRING) {
    INCLUDEPATH += \
        src/PairingManager \
}

#
# Plugin API
#

HEADERS += \
    src/api/QGCCorePlugin.h \
    src/api/QGCOptions.h \
    src/api/QGCSettings.h \
    src/api/QmlComponentInfo.h \
    src/comm/MavlinkMessagesTimer.h \
    src/GPS/Drivers/src/base_station.h \

contains (DEFINES, QGC_ENABLE_PAIRING) {
    HEADERS += \
        src/PairingManager/aes.h
}

SOURCES += \
    src/api/QGCCorePlugin.cc \
    src/api/QGCOptions.cc \
    src/api/QGCSettings.cc \
    src/api/QmlComponentInfo.cc \
    src/comm/MavlinkMessagesTimer.cc \

contains (DEFINES, QGC_ENABLE_PAIRING) {
    SOURCES += \
        src/PairingManager/aes.cpp
}

#
# Unit Test specific configuration goes here (requires full debug build with all plugins)
#

DebugBuild { PX4FirmwarePlugin { PX4FirmwarePluginFactory { APMFirmwarePlugin { APMFirmwarePluginFactory { !MobileBuild {
    DEFINES += UNITTEST_BUILD

    INCLUDEPATH += \
        src/qgcunittest

    HEADERS += \
        src/Audio/AudioOutputTest.h \
        src/FactSystem/FactSystemTestBase.h \
        src/FactSystem/FactSystemTestGeneric.h \
        src/FactSystem/FactSystemTestPX4.h \
        src/FactSystem/ParameterManagerTest.h \
        src/MissionManager/CameraCalcTest.h \
        src/MissionManager/CameraSectionTest.h \
        src/MissionManager/CorridorScanComplexItemTest.h \
        src/MissionManager/FWLandingPatternTest.h \
        src/MissionManager/MissionCommandTreeTest.h \
        src/MissionManager/MissionControllerManagerTest.h \
        src/MissionManager/MissionControllerTest.h \
        src/MissionManager/MissionItemTest.h \
        src/MissionManager/MissionManagerTest.h \
        src/MissionManager/MissionSettingsTest.h \
        src/MissionManager/PlanMasterControllerTest.h \
        src/MissionManager/QGCMapPolygonTest.h \
        src/MissionManager/QGCMapPolylineTest.h \
        src/MissionManager/SectionTest.h \
        src/MissionManager/SimpleMissionItemTest.h \
        src/MissionManager/SpeedSectionTest.h \
        src/MissionManager/StructureScanComplexItemTest.h \
        src/MissionManager/SurveyComplexItemTest.h \
        src/MissionManager/TransectStyleComplexItemTest.h \
        src/MissionManager/VisualMissionItemTest.h \
        src/qgcunittest/GeoTest.h \
        src/qgcunittest/LinkManagerTest.h \
        src/qgcunittest/MavlinkLogTest.h \
        src/qgcunittest/MultiSignalSpy.h \
        src/qgcunittest/TCPLinkTest.h \
        src/qgcunittest/TCPLoopBackServer.h \
        src/qgcunittest/UnitTest.h \
        src/Vehicle/SendMavCommandTest.h \
        #src/qgcunittest/RadioConfigTest.h \
        #src/AnalyzeView/LogDownloadTest.h \
        #src/qgcunittest/FileDialogTest.h \
        #src/qgcunittest/FileManagerTest.h \
        #src/qgcunittest/FlightGearTest.h \
        #src/qgcunittest/MainWindowTest.h \
        #src/qgcunittest/MessageBoxTest.h \

    SOURCES += \
        src/Audio/AudioOutputTest.cc \
        src/FactSystem/FactSystemTestBase.cc \
        src/FactSystem/FactSystemTestGeneric.cc \
        src/FactSystem/FactSystemTestPX4.cc \
        src/FactSystem/ParameterManagerTest.cc \
        src/MissionManager/CameraCalcTest.cc \
        src/MissionManager/CameraSectionTest.cc \
        src/MissionManager/CorridorScanComplexItemTest.cc \
        src/MissionManager/FWLandingPatternTest.cc \
        src/MissionManager/MissionCommandTreeTest.cc \
        src/MissionManager/MissionControllerManagerTest.cc \
        src/MissionManager/MissionControllerTest.cc \
        src/MissionManager/MissionItemTest.cc \
        src/MissionManager/MissionManagerTest.cc \
        src/MissionManager/MissionSettingsTest.cc \
        src/MissionManager/PlanMasterControllerTest.cc \
        src/MissionManager/QGCMapPolygonTest.cc \
        src/MissionManager/QGCMapPolylineTest.cc \
        src/MissionManager/SectionTest.cc \
        src/MissionManager/SimpleMissionItemTest.cc \
        src/MissionManager/SpeedSectionTest.cc \
        src/MissionManager/StructureScanComplexItemTest.cc \
        src/MissionManager/SurveyComplexItemTest.cc \
        src/MissionManager/TransectStyleComplexItemTest.cc \
        src/MissionManager/VisualMissionItemTest.cc \
        src/qgcunittest/GeoTest.cc \
        src/qgcunittest/LinkManagerTest.cc \
        src/qgcunittest/MavlinkLogTest.cc \
        src/qgcunittest/MultiSignalSpy.cc \
        src/qgcunittest/TCPLinkTest.cc \
        src/qgcunittest/TCPLoopBackServer.cc \
        src/qgcunittest/UnitTest.cc \
        src/qgcunittest/UnitTestList.cc \
        src/Vehicle/SendMavCommandTest.cc \
        #src/qgcunittest/RadioConfigTest.cc \
        #src/AnalyzeView/LogDownloadTest.cc \
        #src/qgcunittest/FileDialogTest.cc \
        #src/qgcunittest/FileManagerTest.cc \
        #src/qgcunittest/FlightGearTest.cc \
        #src/qgcunittest/MainWindowTest.cc \
        #src/qgcunittest/MessageBoxTest.cc \

} } } } } }

# Main QGC Headers and Source files

HEADERS += \
    src/ADSB/ADSBVehicle.h \
    src/ADSB/ADSBVehicleManager.h \
    src/AnalyzeView/LogDownloadController.h \
    src/AnalyzeView/PX4LogParser.h \
    src/AnalyzeView/ULogParser.h \
    src/AnalyzeView/MavlinkConsoleController.h \
    src/Audio/AudioOutput.h \
    src/Camera/QGCCameraControl.h \
    src/Camera/QGCCameraIO.h \
    src/Camera/QGCCameraManager.h \
    src/CmdLineOptParser.h \
    src/FirmwarePlugin/PX4/px4_custom_mode.h \
    src/FlightDisplay/VideoManager.h \
    src/FlightMap/Widgets/ValuesWidgetController.h \
    src/FollowMe/FollowMe.h \
    src/Joystick/Joystick.h \
    src/Joystick/JoystickManager.h \
    src/JsonHelper.h \
    src/KMLFileHelper.h \
    src/LogCompressor.h \
    src/MissionManager/CameraCalc.h \
    src/MissionManager/CameraSection.h \
    src/MissionManager/CameraSpec.h \
    src/MissionManager/ComplexMissionItem.h \
    src/MissionManager/CorridorScanComplexItem.h \
    src/MissionManager/CorridorScanPlanCreator.h \
    src/MissionManager/BlankPlanCreator.h \
    src/MissionManager/FixedWingLandingComplexItem.h \
    src/MissionManager/GeoFenceController.h \
    src/MissionManager/GeoFenceManager.h \
    src/MissionManager/KML.h \
    src/MissionManager/MissionCommandList.h \
    src/MissionManager/MissionCommandTree.h \
    src/MissionManager/MissionCommandUIInfo.h \
    src/MissionManager/MissionController.h \
    src/MissionManager/MissionItem.h \
    src/MissionManager/MissionManager.h \
    src/MissionManager/MissionSettingsItem.h \
    src/MissionManager/PlanElementController.h \
    src/MissionManager/PlanCreator.h \
    src/MissionManager/PlanManager.h \
    src/MissionManager/PlanMasterController.h \
    src/MissionManager/QGCFenceCircle.h \
    src/MissionManager/QGCFencePolygon.h \
    src/MissionManager/QGCMapCircle.h \
    src/MissionManager/QGCMapPolygon.h \
    src/MissionManager/QGCMapPolyline.h \
    src/MissionManager/RallyPoint.h \
    src/MissionManager/RallyPointController.h \
    src/MissionManager/RallyPointManager.h \
    src/MissionManager/SimpleMissionItem.h \
    src/MissionManager/Section.h \
    src/MissionManager/SpeedSection.h \
    src/MissionManager/StructureScanComplexItem.h \
    src/MissionManager/StructureScanPlanCreator.h \
    src/MissionManager/SurveyComplexItem.h \
    src/MissionManager/SurveyPlanCreator.h \
    src/MissionManager/TakeoffMissionItem.h \
    src/MissionManager/TransectStyleComplexItem.h \
    src/MissionManager/VisualMissionItem.h \
    src/PositionManager/PositionManager.h \
    src/PositionManager/SimulatedPosition.h \
    src/Geo/QGCGeo.h \
    src/Geo/Constants.hpp \
    src/Geo/Math.hpp \
    src/Geo/Utility.hpp \
    src/Geo/UTMUPS.hpp \
    src/Geo/MGRS.hpp \
    src/Geo/TransverseMercator.hpp \
    src/Geo/PolarStereographic.hpp \
    src/QGC.h \
    src/QGCApplication.h \
    src/QGCComboBox.h \
    src/QGCConfig.h \
    src/QGCFileDownload.h \
    src/QGCLoggingCategory.h \
    src/QGCMapPalette.h \
    src/QGCPalette.h \
    src/QGCQGeoCoordinate.h \
    src/QGCTemporaryFile.h \
    src/QGCToolbox.h \
    src/QmlControls/AppMessages.h \
    src/QmlControls/CoordinateVector.h \
    src/QmlControls/EditPositionDialogController.h \
    src/QmlControls/ParameterEditorController.h \
    src/QmlControls/QGCFileDialogController.h \
    src/QmlControls/QGCImageProvider.h \
    src/QmlControls/QGroundControlQmlGlobal.h \
    src/QmlControls/QmlObjectListModel.h \
    src/QmlControls/QGCGeoBoundingCube.h \
    src/QmlControls/RCChannelMonitorController.h \
    src/QmlControls/ScreenToolsController.h \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.h \
    src/Settings/ADSBVehicleManagerSettings.h \
    src/Settings/AppSettings.h \
    src/Settings/AutoConnectSettings.h \
    src/Settings/BrandImageSettings.h \
    src/Settings/FirmwareUpgradeSettings.h \
    src/Settings/FlightMapSettings.h \
    src/Settings/FlyViewSettings.h \
    src/Settings/OfflineMapsSettings.h \
    src/Settings/PlanViewSettings.h \
    src/Settings/RTKSettings.h \
    src/Settings/SettingsGroup.h \
    src/Settings/SettingsManager.h \
    src/Settings/UnitsSettings.h \
    src/Settings/VideoSettings.h \
    src/ShapeFileHelper.h \
    src/SHPFileHelper.h \
    src/Terrain/TerrainQuery.h \
    src/TerrainTile.h \
    src/Vehicle/GPSRTKFactGroup.h \
    src/Vehicle/MAVLinkLogManager.h \
    src/Vehicle/MultiVehicleManager.h \
    src/Vehicle/TrajectoryPoints.h \
    src/Vehicle/Vehicle.h \
    src/Vehicle/VehicleObjectAvoidance.h \
    src/VehicleSetup/JoystickConfigController.h \
    src/comm/LinkConfiguration.h \
    src/comm/LinkInterface.h \
    src/comm/LinkManager.h \
    src/comm/LogReplayLink.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/QGCMAVLink.h \
    src/comm/TCPLink.h \
    src/comm/UDPLink.h \
    src/comm/UdpIODevice.h \
    src/uas/UAS.h \
    src/uas/UASInterface.h \
    src/uas/UASMessageHandler.h \
    src/AnalyzeView/GeoTagController.h \
    src/AnalyzeView/ExifParser.h \
    src/uas/FileManager.h \

contains (DEFINES, QGC_ENABLE_PAIRING) {
    HEADERS += \
        src/PairingManager/PairingManager.h \
}

AndroidBuild {
HEADERS += \
    src/Joystick/JoystickAndroid.h \
}

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

contains (DEFINES, QGC_ENABLE_PAIRING) {
    contains(DEFINES, QGC_ENABLE_QTNFC) {
        HEADERS += \
            src/PairingManager/QtNFC.h
    }
}

!NoSerialBuild {
HEADERS += \
    src/comm/QGCSerialPortInfo.h \
    src/comm/SerialLink.h \
}

!MobileBuild {
HEADERS += \
    src/GPS/Drivers/src/gps_helper.h \
    src/GPS/Drivers/src/rtcm.h \
    src/GPS/Drivers/src/ashtech.h \
    src/GPS/Drivers/src/ubx.h \
    src/GPS/Drivers/src/sbf.h \
    src/GPS/GPSManager.h \
    src/GPS/GPSPositionMessage.h \
    src/GPS/GPSProvider.h \
    src/GPS/RTCM/RTCMMavlink.h \
    src/GPS/definitions.h \
    src/GPS/satellite_info.h \
    src/GPS/vehicle_gps_position.h \
    src/Joystick/JoystickSDL.h \
    src/RunGuard.h \
    src/comm/QGCHilLink.h \
    src/comm/QGCJSBSimLink.h \
    src/comm/QGCXPlaneLink.h \
}

iOSBuild {
    OBJECTIVE_SOURCES += \
        src/MobileScreenMgr.mm \
}

AndroidBuild {
    SOURCES += src/MobileScreenMgr.cc \
    src/Joystick/JoystickAndroid.cc \
}

SOURCES += \
    src/ADSB/ADSBVehicle.cc \
    src/ADSB/ADSBVehicleManager.cc \
    src/AnalyzeView/LogDownloadController.cc \
    src/AnalyzeView/PX4LogParser.cc \
    src/AnalyzeView/ULogParser.cc \
    src/AnalyzeView/MavlinkConsoleController.cc \
    src/Audio/AudioOutput.cc \
    src/Camera/QGCCameraControl.cc \
    src/Camera/QGCCameraIO.cc \
    src/Camera/QGCCameraManager.cc \
    src/CmdLineOptParser.cc \
    src/FlightDisplay/VideoManager.cc \
    src/FlightMap/Widgets/ValuesWidgetController.cc \
    src/FollowMe/FollowMe.cc \
    src/Joystick/Joystick.cc \
    src/Joystick/JoystickManager.cc \
    src/JsonHelper.cc \
    src/KMLFileHelper.cc \
    src/LogCompressor.cc \
    src/MissionManager/CameraCalc.cc \
    src/MissionManager/CameraSection.cc \
    src/MissionManager/CameraSpec.cc \
    src/MissionManager/ComplexMissionItem.cc \
    src/MissionManager/CorridorScanComplexItem.cc \
    src/MissionManager/CorridorScanPlanCreator.cc \
    src/MissionManager/BlankPlanCreator.cc \
    src/MissionManager/FixedWingLandingComplexItem.cc \
    src/MissionManager/GeoFenceController.cc \
    src/MissionManager/GeoFenceManager.cc \
    src/MissionManager/KML.cc \
    src/MissionManager/MissionCommandList.cc \
    src/MissionManager/MissionCommandTree.cc \
    src/MissionManager/MissionCommandUIInfo.cc \
    src/MissionManager/MissionController.cc \
    src/MissionManager/MissionItem.cc \
    src/MissionManager/MissionManager.cc \
    src/MissionManager/MissionSettingsItem.cc \
    src/MissionManager/PlanElementController.cc \
    src/MissionManager/PlanCreator.cc \
    src/MissionManager/PlanManager.cc \
    src/MissionManager/PlanMasterController.cc \
    src/MissionManager/QGCFenceCircle.cc \
    src/MissionManager/QGCFencePolygon.cc \
    src/MissionManager/QGCMapCircle.cc \
    src/MissionManager/QGCMapPolygon.cc \
    src/MissionManager/QGCMapPolyline.cc \
    src/MissionManager/RallyPoint.cc \
    src/MissionManager/RallyPointController.cc \
    src/MissionManager/RallyPointManager.cc \
    src/MissionManager/SimpleMissionItem.cc \
    src/MissionManager/SpeedSection.cc \
    src/MissionManager/StructureScanComplexItem.cc \
    src/MissionManager/StructureScanPlanCreator.cc \
    src/MissionManager/SurveyComplexItem.cc \
    src/MissionManager/SurveyPlanCreator.cc \
    src/MissionManager/TakeoffMissionItem.cc \
    src/MissionManager/TransectStyleComplexItem.cc \
    src/MissionManager/VisualMissionItem.cc \
    src/PositionManager/PositionManager.cpp \
    src/PositionManager/SimulatedPosition.cc \
    src/Geo/QGCGeo.cc \
    src/Geo/Math.cpp \
    src/Geo/Utility.cpp \
    src/Geo/UTMUPS.cpp \
    src/Geo/MGRS.cpp \
    src/Geo/TransverseMercator.cpp \
    src/Geo/PolarStereographic.cpp \
    src/QGC.cc \
    src/QGCApplication.cc \
    src/QGCComboBox.cc \
    src/QGCFileDownload.cc \
    src/QGCLoggingCategory.cc \
    src/QGCMapPalette.cc \
    src/QGCPalette.cc \
    src/QGCQGeoCoordinate.cc \
    src/QGCTemporaryFile.cc \
    src/QGCToolbox.cc \
    src/QmlControls/AppMessages.cc \
    src/QmlControls/CoordinateVector.cc \
    src/QmlControls/EditPositionDialogController.cc \
    src/QmlControls/ParameterEditorController.cc \
    src/QmlControls/QGCFileDialogController.cc \
    src/QmlControls/QGCImageProvider.cc \
    src/QmlControls/QGroundControlQmlGlobal.cc \
    src/QmlControls/QmlObjectListModel.cc \
    src/QmlControls/QGCGeoBoundingCube.cc \
    src/QmlControls/RCChannelMonitorController.cc \
    src/QmlControls/ScreenToolsController.cc \
    src/QtLocationPlugin/QMLControl/QGCMapEngineManager.cc \
    src/Settings/ADSBVehicleManagerSettings.cc \
    src/Settings/AppSettings.cc \
    src/Settings/AutoConnectSettings.cc \
    src/Settings/BrandImageSettings.cc \
    src/Settings/FirmwareUpgradeSettings.cc \
    src/Settings/FlightMapSettings.cc \
    src/Settings/FlyViewSettings.cc \
    src/Settings/OfflineMapsSettings.cc \
    src/Settings/PlanViewSettings.cc \
    src/Settings/RTKSettings.cc \
    src/Settings/SettingsGroup.cc \
    src/Settings/SettingsManager.cc \
    src/Settings/UnitsSettings.cc \
    src/Settings/VideoSettings.cc \
    src/ShapeFileHelper.cc \
    src/SHPFileHelper.cc \
    src/Terrain/TerrainQuery.cc \
    src/TerrainTile.cc\
    src/Vehicle/GPSRTKFactGroup.cc \
    src/Vehicle/MAVLinkLogManager.cc \
    src/Vehicle/MultiVehicleManager.cc \
    src/Vehicle/TrajectoryPoints.cc \
    src/Vehicle/Vehicle.cc \
    src/Vehicle/VehicleObjectAvoidance.cc \
    src/VehicleSetup/JoystickConfigController.cc \
    src/comm/LinkConfiguration.cc \
    src/comm/LinkInterface.cc \
    src/comm/LinkManager.cc \
    src/comm/LogReplayLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/QGCMAVLink.cc \
    src/comm/TCPLink.cc \
    src/comm/UDPLink.cc \
    src/comm/UdpIODevice.cc \
    src/main.cc \
    src/uas/UAS.cc \
    src/uas/UASMessageHandler.cc \
    src/AnalyzeView/GeoTagController.cc \
    src/AnalyzeView/ExifParser.cc \
    src/uas/FileManager.cc \

contains (DEFINES, QGC_ENABLE_PAIRING) {
    SOURCES += \
        src/PairingManager/PairingManager.cc \
}

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

contains (DEFINES, QGC_ENABLE_PAIRING) {
    contains(DEFINES, QGC_ENABLE_QTNFC) {
        SOURCES += \
        src/PairingManager/QtNFC.cc
    }
}

!MobileBuild {
SOURCES += \
    src/GPS/Drivers/src/gps_helper.cpp \
    src/GPS/Drivers/src/rtcm.cpp \
    src/GPS/Drivers/src/ashtech.cpp \
    src/GPS/Drivers/src/ubx.cpp \
    src/GPS/Drivers/src/sbf.cpp \
    src/GPS/GPSManager.cc \
    src/GPS/GPSProvider.cc \
    src/GPS/RTCM/RTCMMavlink.cc \
    src/Joystick/JoystickSDL.cc \
    src/RunGuard.cc \
    src/comm/QGCJSBSimLink.cc \
    src/comm/QGCXPlaneLink.cc \
}

#
# Firmware Plugin Support
#

INCLUDEPATH += \
    src/AutoPilotPlugins/Common \
    src/FirmwarePlugin \
    src/VehicleSetup \

HEADERS+= \
    src/AutoPilotPlugins/AutoPilotPlugin.h \
    src/AutoPilotPlugins/Common/ESP8266Component.h \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.h \
    src/AutoPilotPlugins/Common/MotorComponent.h \
    src/AutoPilotPlugins/Common/RadioComponentController.h \
    src/AutoPilotPlugins/Common/SyslinkComponent.h \
    src/AutoPilotPlugins/Common/SyslinkComponentController.h \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.h \
    src/FirmwarePlugin/CameraMetaData.h \
    src/FirmwarePlugin/FirmwarePlugin.h \
    src/FirmwarePlugin/FirmwarePluginManager.h \
    src/VehicleSetup/VehicleComponent.h \

!MobileBuild { !NoSerialBuild {
    HEADERS += \
        src/VehicleSetup/Bootloader.h \
        src/VehicleSetup/FirmwareImage.h \
        src/VehicleSetup/FirmwareUpgradeController.h \
        src/VehicleSetup/PX4FirmwareUpgradeThread.h \
}}

SOURCES += \
    src/AutoPilotPlugins/AutoPilotPlugin.cc \
    src/AutoPilotPlugins/Common/ESP8266Component.cc \
    src/AutoPilotPlugins/Common/ESP8266ComponentController.cc \
    src/AutoPilotPlugins/Common/MotorComponent.cc \
    src/AutoPilotPlugins/Common/RadioComponentController.cc \
    src/AutoPilotPlugins/Common/SyslinkComponent.cc \
    src/AutoPilotPlugins/Common/SyslinkComponentController.cc \
    src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc \
    src/FirmwarePlugin/CameraMetaData.cc \
    src/FirmwarePlugin/FirmwarePlugin.cc \
    src/FirmwarePlugin/FirmwarePluginManager.cc \
    src/VehicleSetup/VehicleComponent.cc \

!MobileBuild { !NoSerialBuild {
    SOURCES += \
        src/VehicleSetup/Bootloader.cc \
        src/VehicleSetup/FirmwareImage.cc \
        src/VehicleSetup/FirmwareUpgradeController.cc \
        src/VehicleSetup/PX4FirmwareUpgradeThread.cc \
}}

# ArduPilot Specific

ArdupilotEnabled {
    HEADERS += \
        src/Settings/APMMavlinkStreamRateSettings.h \

    SOURCES += \
        src/Settings/APMMavlinkStreamRateSettings.cc \
}

# ArduPilot FirmwarePlugin

APMFirmwarePlugin {
    RESOURCES *= src/FirmwarePlugin/APM/APMResources.qrc

    INCLUDEPATH += \
        src/AutoPilotPlugins/APM \
        src/FirmwarePlugin/APM \

    HEADERS += \
        src/AutoPilotPlugins/APM/APMAirframeComponent.h \
        src/AutoPilotPlugins/APM/APMAirframeComponentController.h \
        src/AutoPilotPlugins/APM/APMAutoPilotPlugin.h \
        src/AutoPilotPlugins/APM/APMCameraComponent.h \
        src/AutoPilotPlugins/APM/APMCompassCal.h \
        src/AutoPilotPlugins/APM/APMFlightModesComponent.h \
        src/AutoPilotPlugins/APM/APMFlightModesComponentController.h \
        src/AutoPilotPlugins/APM/APMFollowComponent.h \
        src/AutoPilotPlugins/APM/APMFollowComponentController.h \
        src/AutoPilotPlugins/APM/APMHeliComponent.h \
        src/AutoPilotPlugins/APM/APMLightsComponent.h \
        src/AutoPilotPlugins/APM/APMSubFrameComponent.h \
        src/AutoPilotPlugins/APM/APMMotorComponent.h \
        src/AutoPilotPlugins/APM/APMPowerComponent.h \
        src/AutoPilotPlugins/APM/APMRadioComponent.h \
        src/AutoPilotPlugins/APM/APMSafetyComponent.h \
        src/AutoPilotPlugins/APM/APMSensorsComponent.h \
        src/AutoPilotPlugins/APM/APMSensorsComponentController.h \
        src/AutoPilotPlugins/APM/APMSubMotorComponentController.h \
        src/AutoPilotPlugins/APM/APMTuningComponent.h \
        src/FirmwarePlugin/APM/APMFirmwarePlugin.h \
        src/FirmwarePlugin/APM/APMParameterMetaData.h \
        src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.h \
        src/FirmwarePlugin/APM/ArduSubFirmwarePlugin.h \

    SOURCES += \
        src/AutoPilotPlugins/APM/APMAirframeComponent.cc \
        src/AutoPilotPlugins/APM/APMAirframeComponentController.cc \
        src/AutoPilotPlugins/APM/APMAutoPilotPlugin.cc \
        src/AutoPilotPlugins/APM/APMCameraComponent.cc \
        src/AutoPilotPlugins/APM/APMCompassCal.cc \
        src/AutoPilotPlugins/APM/APMFlightModesComponent.cc \
        src/AutoPilotPlugins/APM/APMFlightModesComponentController.cc \
        src/AutoPilotPlugins/APM/APMFollowComponent.cc \
        src/AutoPilotPlugins/APM/APMFollowComponentController.cc \
        src/AutoPilotPlugins/APM/APMHeliComponent.cc \
        src/AutoPilotPlugins/APM/APMLightsComponent.cc \
        src/AutoPilotPlugins/APM/APMSubFrameComponent.cc \
        src/AutoPilotPlugins/APM/APMMotorComponent.cc \
        src/AutoPilotPlugins/APM/APMPowerComponent.cc \
        src/AutoPilotPlugins/APM/APMRadioComponent.cc \
        src/AutoPilotPlugins/APM/APMSafetyComponent.cc \
        src/AutoPilotPlugins/APM/APMSensorsComponent.cc \
        src/AutoPilotPlugins/APM/APMSensorsComponentController.cc \
        src/AutoPilotPlugins/APM/APMSubMotorComponentController.cc \
        src/AutoPilotPlugins/APM/APMTuningComponent.cc \
        src/FirmwarePlugin/APM/APMFirmwarePlugin.cc \
        src/FirmwarePlugin/APM/APMParameterMetaData.cc \
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
    src/FactSystem/FactValueSliderListModel.h \
    src/FactSystem/ParameterManager.h \
    src/FactSystem/SettingsFact.h \

SOURCES += \
    src/FactSystem/Fact.cc \
    src/FactSystem/FactControls/FactPanelController.cc \
    src/FactSystem/FactGroup.cc \
    src/FactSystem/FactMetaData.cc \
    src/FactSystem/FactSystem.cc \
    src/FactSystem/FactValueSliderListModel.cc \
    src/FactSystem/ParameterManager.cc \
    src/FactSystem/SettingsFact.cc \

#-------------------------------------------------------------------------------------
# MAVLink Inspector
contains (DEFINES, QGC_ENABLE_MAVLINK_INSPECTOR) {
    HEADERS += \
        src/AnalyzeView/MAVLinkInspectorController.h
    SOURCES += \
        src/AnalyzeView/MAVLinkInspectorController.cc
    QT += \
        charts
}

#-------------------------------------------------------------------------------------
# Taisync
contains (DEFINES, QGC_GST_TAISYNC_ENABLED) {
    INCLUDEPATH += \
        src/Taisync

    HEADERS += \
        src/Taisync/TaisyncManager.h \
        src/Taisync/TaisyncHandler.h \
        src/Taisync/TaisyncSettings.h \

    SOURCES += \
        src/Taisync/TaisyncManager.cc \
        src/Taisync/TaisyncHandler.cc \
        src/Taisync/TaisyncSettings.cc \

    iOSBuild | AndroidBuild {
        HEADERS += \
            src/Taisync/TaisyncTelemetry.h \
            src/Taisync/TaisyncVideoReceiver.h \

        SOURCES += \
            src/Taisync/TaisyncTelemetry.cc \
            src/Taisync/TaisyncVideoReceiver.cc \
    }
}

#-------------------------------------------------------------------------------------
# Microhard
contains (DEFINES, QGC_GST_MICROHARD_ENABLED) {
    INCLUDEPATH += \
        src/Microhard

    HEADERS += \
        src/Microhard/MicrohardManager.h \
        src/Microhard/MicrohardHandler.h \
        src/Microhard/MicrohardSettings.h \

    SOURCES += \
        src/Microhard/MicrohardManager.cc \
        src/Microhard/MicrohardHandler.cc \
        src/Microhard/MicrohardSettings.cc \
}

#-------------------------------------------------------------------------------------
# AirMap

contains (DEFINES, QGC_AIRMAP_ENABLED) {

    #-- These should be always enabled but not yet
    INCLUDEPATH += \
        src/AirspaceManagement

    HEADERS += \
        src/AirspaceManagement/AirspaceAdvisoryProvider.h \
        src/AirspaceManagement/AirspaceFlightPlanProvider.h \
        src/AirspaceManagement/AirspaceManager.h \
        src/AirspaceManagement/AirspaceRestriction.h \
        src/AirspaceManagement/AirspaceRestrictionProvider.h \
        src/AirspaceManagement/AirspaceRulesetsProvider.h \
        src/AirspaceManagement/AirspaceVehicleManager.h \
        src/AirspaceManagement/AirspaceWeatherInfoProvider.h \

    SOURCES += \
        src/AirspaceManagement/AirspaceAdvisoryProvider.cc \
        src/AirspaceManagement/AirspaceFlightPlanProvider.cc \
        src/AirspaceManagement/AirspaceManager.cc \
        src/AirspaceManagement/AirspaceRestriction.cc \
        src/AirspaceManagement/AirspaceRestrictionProvider.cc \
        src/AirspaceManagement/AirspaceRulesetsProvider.cc \
        src/AirspaceManagement/AirspaceVehicleManager.cc \
        src/AirspaceManagement/AirspaceWeatherInfoProvider.cc \

    #-- This is the AirMap implementation of the above
    RESOURCES += \
        src/Airmap/airmap.qrc

    INCLUDEPATH += \
        src/Airmap

    HEADERS += \
        src/Airmap/AirMapAdvisoryManager.h \
        src/Airmap/AirMapFlightManager.h \
        src/Airmap/AirMapFlightPlanManager.h \
        src/Airmap/AirMapManager.h \
        src/Airmap/AirMapRestrictionManager.h \
        src/Airmap/AirMapRulesetsManager.h \
        src/Airmap/AirMapSettings.h \
        src/Airmap/AirMapSharedState.h \
        src/Airmap/AirMapTelemetry.h \
        src/Airmap/AirMapTrafficMonitor.h \
        src/Airmap/AirMapVehicleManager.h \
        src/Airmap/AirMapWeatherInfoManager.h \
        src/Airmap/LifetimeChecker.h \

    SOURCES += \
        src/Airmap/AirMapAdvisoryManager.cc \
        src/Airmap/AirMapFlightManager.cc \
        src/Airmap/AirMapFlightPlanManager.cc \
        src/Airmap/AirMapManager.cc \
        src/Airmap/AirMapRestrictionManager.cc \
        src/Airmap/AirMapRulesetsManager.cc \
        src/Airmap/AirMapSettings.cc \
        src/Airmap/AirMapSharedState.cc \
        src/Airmap/AirMapTelemetry.cc \
        src/Airmap/AirMapTrafficMonitor.cc \
        src/Airmap/AirMapVehicleManager.cc \
        src/Airmap/AirMapWeatherInfoManager.cc \

    #-- Do we have an API key?
    exists(src/Airmap/Airmap_api_key.h) {
        message("Using compile time Airmap API key")
        HEADERS += \
            src/Airmap/Airmap_api_key.h
        DEFINES += QGC_AIRMAP_KEY_AVAILABLE
    }

    include(src/Airmap/QJsonWebToken/src/qjsonwebtoken.pri)

} else {
    #-- Dummies
    INCLUDEPATH += \
        src/Airmap/dummy
    RESOURCES += \
        src/Airmap/dummy/airmap_dummy.qrc
    HEADERS += \
        src/Airmap/dummy/AirspaceManager.h
    SOURCES += \
        src/Airmap/dummy/AirspaceManager.cc
}

#-------------------------------------------------------------------------------------
# Video Streaming

INCLUDEPATH += \
    src/VideoStreaming

HEADERS += \
    src/VideoStreaming/VideoReceiver.h \
    src/VideoStreaming/VideoStreaming.h \
    src/VideoStreaming/SubtitleWriter.h \

SOURCES += \
    src/VideoStreaming/VideoReceiver.cc \
    src/VideoStreaming/VideoStreaming.cc \
    src/VideoStreaming/SubtitleWriter.cc \

contains (CONFIG, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from command line)")
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_VIDEOSTREAMING) {
    message("Skipping support for video streaming (manual override from user_config.pri)")
} else {
    include(src/VideoStreaming/VideoStreaming.pri)
}

!VideoEnabled {
    HEADERS += \
       src/VideoStreaming/GLVideoItemStub.h
    SOURCES += \
        src/VideoStreaming/GLVideoItemStub.cc
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
# Localization
#

TRANSLATIONS += $$files($$PWD/localization/qgc_*.ts)
CONFIG+=lrelease embed_translations

#-------------------------------------------------------------------------------------
#
# Post link configuration
#

contains (CONFIG, QGC_DISABLE_BUILD_SETUP) {
    message("Disable standard build setup")
} else {
    include(QGCSetup.pri)
}

#
# Installer targets
#

contains (CONFIG, QGC_DISABLE_INSTALLER_SETUP) {
    message("Disable standard installer setup")
} else {
    include(QGCInstaller.pri)
}
