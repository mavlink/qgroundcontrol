# Include QMapControl map library
# prefer version from external directory /
# from http://github.com/pixhawk/qmapcontrol/
# over bundled version in lib directory
# Version from GIT repository is preferred
# include ( "../qmapcontrol/QMapControl/QMapControl.pri" ) #{
# Include bundled version if necessary
include(lib/QMapControl/QMapControl.pri)

# message("Including bundled QMapControl version as FALLBACK. This is fine on Linux and MacOS, but not the best choice in Windows")
QT += network \
    opengl \
    svg \
    xml \
    phonon
TEMPLATE = app
TARGET = qgroundcontrol
BASEDIR = .
BUILDDIR = build
LANGUAGE = C++
CONFIG += debug_and_release \
    console
OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated

# }
# Include general settings for MAVGround
# necessary as last include to override any non-acceptable settings
# done by the plugins above
include(qgroundcontrol.pri)

# QWT plot and QExtSerial depend on paths set by qgroundcontrol.pri
# Include serial port library
include(src/lib/qextserialport/qextserialport.pri)

# Include QWT plotting library
include(src/lib/qwt/qwt.pri)
DEPENDPATH += . \
    lib/QMapControl \
    lib/QMapControl/src \
    plugins
INCLUDEPATH += . \
    lib/QMapControl \
    $$BASEDIR/../mavlink/contrib/slugs/include \
    $$BASEDIR/../mavlink/include

# ../mavlink/include \
# MAVLink/include \
# mavlink/include
# Input
FORMS += src/ui/MainWindow.ui \
    src/ui/CommSettings.ui \
    src/ui/SerialSettings.ui \
    src/ui/UASControl.ui \
    src/ui/UASList.ui \
    src/ui/UASInfo.ui \
    src/ui/LineChart.ui \
    src/ui/UASView.ui \
    src/ui/ParameterInterface.ui \
    src/ui/WaypointList.ui \
    src/ui/WaypointView.ui \
    src/ui/ObjectDetectionView.ui \
    src/ui/JoystickWidget.ui \
    src/ui/DebugConsole.ui \
    src/ui/MapWidget.ui \
    src/ui/XMLCommProtocolWidget.ui \
    src/ui/HDDisplay.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/AudioOutputWidget.ui \
    src/ui/QGCSensorSettingsWidget.ui \
    src/ui/watchdog/WatchdogControl.ui \
    src/ui/watchdog/WatchdogProcessView.ui \
    src/ui/watchdog/WatchdogView.ui \
    src/ui/QGCFirmwareUpdate.ui \
    src/ui/QGCPxImuFirmwareUpdate.ui \
    src/ui/QGCDataPlot2D.ui
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
    src/ui/watchdog
HEADERS += src/MG.h \
    src/Core.h \
    src/uas/UASInterface.h \
    src/uas/UAS.h \
    src/uas/UASManager.h \
    src/comm/LinkManager.h \
    src/comm/LinkInterface.h \
    src/comm/SerialLinkInterface.h \
    src/comm/SerialLink.h \
    src/comm/SerialSimulationLink.h \
    src/comm/ProtocolInterface.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/AS4Protocol.h \
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
    src/ui/WaypointView.h \
    src/ui/ObjectDetectionView.h \
    src/input/JoystickInput.h \
    src/ui/JoystickWidget.h \
    src/ui/DebugConsole.h \
    src/ui/MapWidget.h \
    src/ui/XMLCommProtocolWidget.h \
    src/ui/mavlink/DomItem.h \
    src/ui/mavlink/DomModel.h \
    src/comm/MAVLinkXMLParser.h \
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
    src/uas/ArduPilotMAV.h \
    src/comm/MAVLinkSyntaxHighlighter.h \
    src/ui/watchdog/WatchdogControl.h \
    src/ui/watchdog/WatchdogProcessView.h \
    src/ui/watchdog/WatchdogView.h \
    src/uas/UASWaypointManager.h \
    src/ui/HSIDisplay.h \
    src/QGC.h \
    src/ui/QGCFirmwareUpdate.h \
    src/ui/QGCPxImuFirmwareUpdate.h \
    src/comm/MAVLinkLightProtocol.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/linechart/IncrementalPlot.h
SOURCES += src/main.cc \
    src/Core.cc \
    src/uas/UASManager.cc \
    src/uas/UAS.cc \
    src/comm/LinkManager.cc \
    src/comm/SerialLink.cc \
    src/comm/SerialSimulationLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/AS4Protocol.cc \
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
    src/ui/WaypointView.cc \
    src/ui/ObjectDetectionView.cc \
    src/input/JoystickInput.cc \
    src/ui/JoystickWidget.cc \
    src/ui/DebugConsole.cc \
    src/ui/MapWidget.cc \
    src/ui/XMLCommProtocolWidget.cc \
    src/ui/mavlink/DomItem.cc \
    src/ui/mavlink/DomModel.cc \
    src/comm/MAVLinkXMLParser.cc \
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
    src/uas/ArduPilotMAV.cc \
    src/comm/MAVLinkSyntaxHighlighter.cc \
    src/ui/watchdog/WatchdogControl.cc \
    src/ui/watchdog/WatchdogProcessView.cc \
    src/ui/watchdog/WatchdogView.cc \
    src/uas/UASWaypointManager.cc \
    src/ui/HSIDisplay.cc \
    src/QGC.cc \
    src/ui/QGCFirmwareUpdate.cc \
    src/ui/QGCPxImuFirmwareUpdate.cc \
    src/comm/MAVLinkLightProtocol.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/linechart/IncrementalPlot.cc
RESOURCES = mavground.qrc

# Include RT-LAB Library
win32 { 
    LIBS += -LC:\OPAL-RT\RT-LAB7.2.4\Common\bin \
        -lOpalApi
    INCLUDEPATH += src/lib/opalrt
    SOURCES += src/comm/OpalLink.cc
    HEADERS += src/comm/OpalLink.h
    DEFINES += OPAL_RT
}
