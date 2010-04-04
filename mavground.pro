# Include general settings for MAVGround
include(mavground.pri)

# Include serial port library
include(src/lib/qextserialport/qextserialport.pri)

# Include QWT plotting library
include(src/lib/qwt/qwt.pri)

# Include FLITE audio synthesizer library
#include(src/lib/flite/flite.pri)

# Include QMapControl map library
include(lib/QMapControl/QMapControl.pri)
DEPENDPATH += . \
    lib/QMapControl \
    lib/QMapControl/src
INCLUDEPATH += . \
    lib/QMapControl \
    ../mavlink/src

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
    src/ui/AudioOutputWidget.ui
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
    src/ui/mavlink
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
    src/ui/linechart/LinechartContainer.h \
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
    src/ui/PFD.h \
    src/ui/GaugePanel.h \
    src/ui/DebugConsole.h \
    src/ui/MapWidget.h \
    src/ui/XMLCommProtocolWidget.h \
    src/ui/mavlink/DomItem.h \
    src/ui/mavlink/DomModel.h \
    src/comm/MAVLinkXMLParser.h \
    src/ui/HDDisplay.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/ui/AudioOutputWidget.h \
    src/AudioOutput.h \
    src/LogCompressor.h \
    src/comm/ViconTarsusProtocol.h \
    src/comm/TarsusField.h \
    src/comm/TarsusFieldDescriptor.h \
    src/comm/TarsusFieldTriplet.h \
    src/comm/TarsusRigidBody.h \
    src/comm/TarsusStream.h
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
    src/ui/linechart/LinechartContainer.cc \
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
    src/ui/PFD.cc \
    src/ui/GaugePanel.cc \
    src/ui/DebugConsole.cc \
    src/ui/MapWidget.cc \
    src/ui/XMLCommProtocolWidget.cc \
    src/ui/mavlink/DomItem.cc \
    src/ui/mavlink/DomModel.cc \
    src/comm/MAVLinkXMLParser.cc \
    src/ui/HDDisplay.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/ui/AudioOutputWidget.cc \
    src/AudioOutput.cc \
    src/LogCompressor.cc \
    src/comm/ViconTarsusProtocol.cc
RESOURCES = mavground.qrc
