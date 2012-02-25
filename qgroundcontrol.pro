# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2011 QGroundControl Developers
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


# Qt configuration
CONFIG += qt \
    thread
QT += network \
    opengl \
    svg \
    xml \
    phonon \
    webkit \
    sql

TEMPLATE = app
TARGET = qgroundcontrol
BASEDIR = $${IN_PWD}
linux-g++|linux-g++-64{
    debug {
        TARGETDIR = $${OUT_PWD}/debug
        BUILDDIR = $${OUT_PWD}/build-debug
    }
    release {
        TARGETDIR = $${OUT_PWD}/release
        BUILDDIR = $${OUT_PWD}/build-release
    }
} else {
    TARGETDIR = $${OUT_PWD}
    BUILDDIR = $${OUT_PWD}/build
}
LANGUAGE = C++
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
UI_DIR = $${BUILDDIR}/ui
RCC_DIR = $${BUILDDIR}/rcc
MAVLINK_CONF = ""
MAVLINKPATH = $$BASEDIR/thirdParty/mavlink/include
DEFINES += MAVLINK_NO_DATA

win32 {
    QMAKE_INCDIR_QT = $$(QTDIR)/include
    QMAKE_LIBDIR_QT = $$(QTDIR)/lib
    QMAKE_UIC = "$$(QTDIR)/bin/uic.exe"
    QMAKE_MOC = "$$(QTDIR)/bin/moc.exe"
    QMAKE_RCC = "$$(QTDIR)/bin/rcc.exe"
    QMAKE_QMAKE = "$$(QTDIR)/bin/qmake.exe"
}



#################################################################
# EXTERNAL LIBRARY CONFIGURATION

# Include NMEA parsing library (currently unused)
include(src/libs/nmea/nmea.pri)

# EIGEN matrix library (header-only)
INCLUDEPATH += src/libs/eigen

# OPMapControl library (from OpenPilot)
include(src/libs/utils/utils_external.pri)
include(src/libs/opmapcontrol/opmapcontrol_external.pri)
DEPENDPATH += \
    src/libs/utils \
    src/libs/utils/src \
    src/libs/opmapcontrol \
    src/libs/opmapcontrol/src \
    src/libs/opmapcontrol/src/mapwidget

INCLUDEPATH += \
    src/libs/utils \
    src/libs \
    src/libs/opmapcontrol

# If the user config file exists, it will be included.
# if the variable MAVLINK_CONF contains the name of an
# additional project, QGroundControl includes the support
# of custom MAVLink messages of this project
exists(user_config.pri) { 
    include(user_config.pri)
    message("----- USING CUSTOM USER QGROUNDCONTROL CONFIG FROM user_config.pri -----")
    message("Adding support for additional MAVLink messages for: " $$MAVLINK_CONF)
    message("------------------------------------------------------------------------")
}
INCLUDEPATH += $$MAVLINKPATH/common
INCLUDEPATH += $$MAVLINKPATH
contains(MAVLINK_CONF, pixhawk) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common

    # PIXHAWK SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/pixhawk
    DEFINES += QGC_USE_PIXHAWK_MESSAGES
}
contains(MAVLINK_CONF, slugs) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common
    
    # SLUGS SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/slugs
    DEFINES += QGC_USE_SLUGS_MESSAGES
}
contains(MAVLINK_CONF, ualberta) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common
    
    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/ualberta
    DEFINES += QGC_USE_UALBERTA_MESSAGES
}
contains(MAVLINK_CONF, ardupilotmega) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common
    INCLUDEPATH -= $$BASEDIR/thirdParty/mavlink/include/common
    
    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/ardupilotmega
    DEFINES += QGC_USE_ARDUPILOTMEGA_MESSAGES
}
contains(MAVLINK_CONF, senseSoar) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$MAVLINKPATH/common
    
    # SENSESOAR SPECIAL MESSAGES
    INCLUDEPATH += $$MAVLINKPATH/SenseSoar
    DEFINES += QGC_USE_SENSESOAR_MESSAGES
}


# Include general settings for QGroundControl
# necessary as last include to override any non-acceptable settings
# done by the plugins above
include(qgroundcontrol.pri)

# Include MAVLink generator
# has been deprecated
DEPENDPATH += \
    src/apps/mavlinkgen

INCLUDEPATH += \
    src/apps/mavlinkgen \
    src/apps/mavlinkgen/ui \
    src/apps/mavlinkgen/generator

include(src/apps/mavlinkgen/mavlinkgen.pri)



# Include QWT plotting library
include(src/lib/qwt/qwt.pri)
DEPENDPATH += . \
    plugins \
    thirdParty/qserialport/include \
    thirdParty/qserialport/include/QtSerialPort \
    thirdParty/qserialport \
    src/libs/qextserialport

INCLUDEPATH += . \
    thirdParty/qserialport/include \
    thirdParty/qserialport/include/QtSerialPort \
    thirdParty/qserialport/src \
    src/libs/qextserialport

# Include serial port library (QSerial)
include(qserialport.pri)

# Serial port detection (ripped-off from qextserialport library)
macx|macx-g++|macx-g++42::SOURCES += src/libs/qextserialport/qextserialenumerator_osx.cpp
linux-g++::SOURCES += src/libs/qextserialport/qextserialenumerator_unix.cpp
linux-g++-64::SOURCES += src/libs/qextserialport/qextserialenumerator_unix.cpp
win32::SOURCES += src/libs/qextserialport/qextserialenumerator_win.cpp
win32-msvc2008|win32-msvc2010::SOURCES += src/libs/qextserialport/qextserialenumerator_win.cpp

# Input
FORMS += src/ui/MainWindow.ui \
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
    src/ui/mission/QGCCustomWaypointAction.ui \
    src/ui/QGCUDPLinkConfiguration.ui \
    src/ui/QGCSettingsWidget.ui \
    src/ui/UASControlParameters.ui \
    src/ui/mission/QGCMissionDoWidget.ui \
    src/ui/mission/QGCMissionConditionWidget.ui \
    src/ui/map/QGCMapTool.ui \
    src/ui/map/QGCMapToolBar.ui \
    src/ui/QGCMAVLinkInspector.ui \
    src/ui/WaypointViewOnlyView.ui \    
    src/ui/WaypointEditableView.ui \    
    src/ui/UnconnectedUASInfoWidget.ui \
    src/ui/mavlink/QGCMAVLinkMessageSender.ui \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.ui \
    src/ui/QGCPluginHost.ui \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.ui
INCLUDEPATH += src \
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
    src/ui/designer
HEADERS += src/MG.h \
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
    src/ui/QGCUDPLinkConfiguration.h \
    src/ui/QGCSettingsWidget.h \
    src/ui/uas/UASControlParameters.h \
    src/ui/mission/QGCMissionDoWidget.h \
    src/ui/mission/QGCMissionConditionWidget.h \
    src/uas/QGCUASParamManager.h \
    src/ui/map/QGCMapWidget.h \
    src/ui/map/MAV2DIcon.h \
    src/ui/map/Waypoint2DIcon.h \
    src/ui/map/QGCMapTool.h \
    src/ui/map/QGCMapToolBar.h \
    src/libs/qextserialport/qextserialenumerator.h \
    src/QGCGeo.h \
    src/ui/QGCToolBar.h \
    src/ui/QGCMAVLinkInspector.h \
    src/ui/MAVLinkDecoder.h \
    src/ui/WaypointViewOnlyView.h \
    src/ui/WaypointViewOnlyView.h \
    src/ui/WaypointEditableView.h \    
    src/ui/UnconnectedUASInfoWidget.h \
    src/ui/QGCRGBDView.h \
    src/ui/mavlink/QGCMAVLinkMessageSender.h \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.h \
    src/ui/QGCPluginHost.h \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.h

# Google Earth is only supported on Mac OS and Windows with Visual Studio Compiler
macx|macx-g++|macx-g++42|win32-msvc2008|win32-msvc2010::HEADERS += src/ui/map3D/QGCGoogleEarthView.h
contains(DEPENDENCIES_PRESENT, osg) { 
    message("Including headers for OpenSceneGraph")
    
    # Enable only if OpenSceneGraph is available
    HEADERS += src/ui/map3D/gpl.h \
        src/ui/map3D/CameraParams.h \
        src/ui/map3D/ViewParamWidget.h \
        src/ui/map3D/SystemContainer.h \
        src/ui/map3D/SystemViewParams.h \
        src/ui/map3D/GlobalViewParams.h \
        src/ui/map3D/SystemGroupNode.h \
        src/ui/map3D/Q3DWidget.h \
        src/ui/map3D/GCManipulator.h \
        src/ui/map3D/ImageWindowGeode.h \
        src/ui/map3D/PixhawkCheetahGeode.h \
        src/ui/map3D/Pixhawk3DWidget.h \
        src/ui/map3D/Q3DWidgetFactory.h \
        src/ui/map3D/WebImageCache.h \
        src/ui/map3D/WebImage.h \
        src/ui/map3D/TextureCache.h \
        src/ui/map3D/Texture.h \
        src/ui/map3D/Imagery.h \
        src/ui/map3D/HUDScaleGeode.h \
        src/ui/map3D/WaypointGroupNode.h \
        src/ui/map3D/TerrainParamDialog.h \
        src/ui/map3D/ImageryParamDialog.h
}
contains(DEPENDENCIES_PRESENT, protobuf):contains(MAVLINK_CONF, pixhawk) {
    message("Including headers for Protocol Buffers")

    # Enable only if protobuf is available
    HEADERS += thirdParty/mavlink/include/pixhawk/pixhawk.pb.h \
        src/ui/map3D/ObstacleGroupNode.h \
        src/ui/map3D/GLOverlayGeode.h
}
contains(DEPENDENCIES_PRESENT, libfreenect) { 
    message("Including headers for libfreenect")
    
    # Enable only if libfreenect is available
    HEADERS += src/input/Freenect.h
}
SOURCES += src/main.cc \
    src/QGCCore.cc \
    src/uas/UASManager.cc \
    src/uas/UAS.cc \
    src/comm/LinkManager.cc \
    src/comm/LinkInterface.cpp \
    src/comm/SerialLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/QGCFlightGearLink.cc \
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
    src/ui/QGCUDPLinkConfiguration.cc \
    src/ui/QGCSettingsWidget.cc \
    src/ui/uas/UASControlParameters.cpp \
    src/ui/mission/QGCMissionDoWidget.cc \
    src/ui/mission/QGCMissionConditionWidget.cc \
    src/uas/QGCUASParamManager.cc \
    src/ui/map/QGCMapWidget.cc \
    src/ui/map/MAV2DIcon.cc \
    src/ui/map/Waypoint2DIcon.cc \
    src/ui/map/QGCMapTool.cc \
    src/ui/map/QGCMapToolBar.cc \
    src/ui/QGCToolBar.cc \
    src/ui/QGCMAVLinkInspector.cc \
    src/ui/MAVLinkDecoder.cc \
    src/ui/WaypointViewOnlyView.cc \
    src/ui/WaypointEditableView.cc \
    src/ui/UnconnectedUASInfoWidget.cc \
    src/ui/QGCRGBDView.cc \
    src/ui/mavlink/QGCMAVLinkMessageSender.cc \
    src/ui/firmwareupdate/QGCFirmwareUpdateWidget.cc \
    src/ui/QGCPluginHost.cc \
    src/ui/firmwareupdate/QGCPX4FirmwareUpdate.cc

# Enable Google Earth only on Mac OS and Windows with Visual Studio compiler
macx|macx-g++|macx-g++42|win32-msvc2008|win32-msvc2010::SOURCES += src/ui/map3D/QGCGoogleEarthView.cc

# Enable OSG only if it has been found
contains(DEPENDENCIES_PRESENT, osg) { 
    message("Including sources for OpenSceneGraph")
    
    # Enable only if OpenSceneGraph is available
    SOURCES += src/ui/map3D/gpl.cc \
        src/ui/map3D/CameraParams.cc \
        src/ui/map3D/ViewParamWidget.cc \
        src/ui/map3D/SystemContainer.cc \
        src/ui/map3D/SystemViewParams.cc \
        src/ui/map3D/GlobalViewParams.cc \
        src/ui/map3D/SystemGroupNode.cc \
        src/ui/map3D/Q3DWidget.cc \
        src/ui/map3D/ImageWindowGeode.cc \
        src/ui/map3D/GCManipulator.cc \
        src/ui/map3D/PixhawkCheetahGeode.cc \
        src/ui/map3D/Pixhawk3DWidget.cc \
        src/ui/map3D/Q3DWidgetFactory.cc \
        src/ui/map3D/WebImageCache.cc \
        src/ui/map3D/WebImage.cc \
        src/ui/map3D/TextureCache.cc \
        src/ui/map3D/Texture.cc \
        src/ui/map3D/Imagery.cc \
        src/ui/map3D/HUDScaleGeode.cc \
        src/ui/map3D/WaypointGroupNode.cc \
        src/ui/map3D/TerrainParamDialog.cc \
        src/ui/map3D/ImageryParamDialog.cc

    contains(DEPENDENCIES_PRESENT, osgearth) { 
        message("Including sources for osgEarth")
        
        # Enable only if OpenSceneGraph is available
        SOURCES += src/ui/map3D/QMap3D.cc
    }
}
contains(DEPENDENCIES_PRESENT, protobuf):contains(MAVLINK_CONF, pixhawk) {
    message("Including sources for Protocol Buffers")

    # Enable only if protobuf is available
    SOURCES += thirdParty/mavlink/src/pixhawk/pixhawk.pb.cc \
        src/ui/map3D/ObstacleGroupNode.cc \
        src/ui/map3D/GLOverlayGeode.cc
}
contains(DEPENDENCIES_PRESENT, libfreenect) { 
    message("Including sources for libfreenect")
    
    # Enable only if libfreenect is available
    SOURCES += src/input/Freenect.cc
}

# Add icons and other resources
RESOURCES += qgroundcontrol.qrc

# Include RT-LAB Library
win32:exists(src/lib/opalrt/OpalApi.h):exists(C:/OPAL-RT/RT-LAB7.2.4/Common/bin) { 
    message("Building support for Opal-RT")
    LIBS += -LC:/OPAL-RT/RT-LAB7.2.4/Common/bin \
        -lOpalApi
    INCLUDEPATH += src/lib/opalrt
    HEADERS += src/comm/OpalRT.h \
        src/comm/OpalLink.h \
        src/comm/Parameter.h \
        src/comm/QGCParamID.h \
        src/comm/ParameterList.h \
        src/ui/OpalLinkConfigurationWindow.h
    SOURCES += src/comm/OpalRT.cc \
        src/comm/OpalLink.cc \
        src/comm/Parameter.cc \
        src/comm/QGCParamID.cc \
        src/comm/ParameterList.cc \
        src/ui/OpalLinkConfigurationWindow.cc
    FORMS += src/ui/OpalLinkSettings.ui
    DEFINES += OPAL_RT
}
TRANSLATIONS += es-MX.ts \
    en-US.ts

# xbee support
# libxbee only supported by linux and windows systems
win32-msvc2008|win32-msvc2010|linux {
    HEADERS += src/comm/XbeeLinkInterface.h \
        src/comm/XbeeLink.h \
        src/comm/HexSpinBox.h \
        src/ui/XbeeConfigurationWindow.h \
        src/comm/CallConv.h
    SOURCES += src/comm/XbeeLink.cpp \
        src/comm/HexSpinBox.cpp \
        src/ui/XbeeConfigurationWindow.cpp
    DEFINES += XBEELINK
    INCLUDEPATH += thirdParty/libxbee
# TO DO: build library when it does not exist already
    LIBS += -LthirdParty/libxbee/lib \
        -llibxbee
}
