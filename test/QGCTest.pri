################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

#  testlib is needed even in release flavor for QSignalSpy support
QT += testlib
ReleaseBuild {
    QT.testlib.CONFIG -= console
} else:DebugBuild {
    RESOURCES += $$PWD/UnitTest.qrc
    DEFINES += UNITTEST_BUILD

    INCLUDEPATH += \
        $$PWD/AnalyzeView \
        $$PWD/Audio \
        $$PWD/AutoPilotPlugins \
        $$PWD/Comms \
        $$PWD/FactSystem \
        $$PWD/Geo \
        $$PWD/MAVLink \
        $$PWD/MissionManager \
        $$PWD/qgcunittest \
        $$PWD/QmlControls \
        $$PWD/Terrain \
        $$PWD/UI \
        $$PWD/Utilities/Compression \
        $$PWD/Vehicle

    HEADERS += \
        #$$PWD/AnalyzeView/LogDownloadTest.h \
        $$PWD/Audio/AudioOutputTest.h \
        $$PWD/Utilities/Compression/DecompressionTest.h \
        $$PWD/FactSystem/FactSystemTestBase.h \
        $$PWD/FactSystem/FactSystemTestGeneric.h \
        $$PWD/FactSystem/FactSystemTestPX4.h \
        $$PWD/FactSystem/ParameterManagerTest.h \
        $$PWD/Geo/GeoTest.h \
        $$PWD/MissionManager/CameraCalcTest.h \
        $$PWD/MissionManager/CameraSectionTest.h \
        $$PWD/MissionManager/CorridorScanComplexItemTest.h \
        $$PWD/MissionManager/FWLandingPatternTest.h \
        $$PWD/MissionManager/LandingComplexItemTest.h \
        $$PWD/MissionManager/MissionCommandTreeEditorTest.h \
        $$PWD/MissionManager/MissionCommandTreeTest.h \
        $$PWD/MissionManager/MissionControllerManagerTest.h \
        $$PWD/MissionManager/MissionControllerTest.h \
        $$PWD/MissionManager/MissionItemTest.h \
        $$PWD/MissionManager/MissionManagerTest.h \
        $$PWD/MissionManager/MissionSettingsTest.h \
        $$PWD/MissionManager/PlanMasterControllerTest.h \
        $$PWD/MissionManager/QGCMapPolygonTest.h \
        $$PWD/MissionManager/QGCMapPolylineTest.h \
        $$PWD/MissionManager/SectionTest.h \
        $$PWD/MissionManager/SimpleMissionItemTest.h \
        $$PWD/MissionManager/SpeedSectionTest.h \
        $$PWD/MissionManager/StructureScanComplexItemTest.h \
        $$PWD/MissionManager/SurveyComplexItemTest.h \
        $$PWD/MissionManager/TransectStyleComplexItemTest.h \
        $$PWD/MissionManager/TransectStyleComplexItemTestBase.h \
        $$PWD/MissionManager/VisualMissionItemTest.h \
        #$$PWD/qgcunittest/FileDialogTest.h \
        #$$PWD/qgcunittest/MainWindowTest.h \
        #$$PWD/qgcunittest/MessageBoxTest.h \
        #$$PWD/AutoPilotPlugins/RadioConfigTest.h \
        $$PWD/Vehicle/Components/ComponentInformationCacheTest.h \
        $$PWD/Vehicle/Components/ComponentInformationTranslationTest.h \
        $$PWD/AnalyzeView/MavlinkLogTest.h \
        $$PWD/qgcunittest/MultiSignalSpy.h \
        $$PWD/qgcunittest/MultiSignalSpyV2.h \
        $$PWD/qgcunittest/UnitTest.h \
        $$PWD/Terrain/TerrainQueryTest.h \
        $$PWD/Vehicle/FTPManagerTest.h \
        $$PWD/Vehicle/InitialConnectTest.h \
        $$PWD/Vehicle/RequestMessageTest.h \
        $$PWD/Vehicle/SendMavCommandWithHandlerTest.h \
        $$PWD/Vehicle/SendMavCommandWithSignallingTest.h \
        $$PWD/Vehicle/VehicleLinkManagerTest.h \

    SOURCES += \
        #$$PWD/AnalyzeView/LogDownloadTest.cc \
        $$PWD/Audio/AudioOutputTest.cc \
        $$PWD/Utilities/Compression/DecompressionTest.cc \
        $$PWD/FactSystem/FactSystemTestBase.cc \
        $$PWD/FactSystem/FactSystemTestGeneric.cc \
        $$PWD/FactSystem/FactSystemTestPX4.cc \
        $$PWD/FactSystem/ParameterManagerTest.cc \
        $$PWD/Geo/GeoTest.cc \
        $$PWD/MissionManager/CameraCalcTest.cc \
        $$PWD/MissionManager/CameraSectionTest.cc \
        $$PWD/MissionManager/CorridorScanComplexItemTest.cc \
        $$PWD/MissionManager/FWLandingPatternTest.cc \
        $$PWD/MissionManager/LandingComplexItemTest.cc \
        $$PWD/MissionManager/MissionCommandTreeEditorTest.cc \
        $$PWD/MissionManager/MissionCommandTreeTest.cc \
        $$PWD/MissionManager/MissionControllerManagerTest.cc \
        $$PWD/MissionManager/MissionControllerTest.cc \
        $$PWD/MissionManager/MissionItemTest.cc \
        $$PWD/MissionManager/MissionManagerTest.cc \
        $$PWD/MissionManager/MissionSettingsTest.cc \
        $$PWD/MissionManager/PlanMasterControllerTest.cc \
        $$PWD/MissionManager/QGCMapPolygonTest.cc \
        $$PWD/MissionManager/QGCMapPolylineTest.cc \
        $$PWD/MissionManager/SectionTest.cc \
        $$PWD/MissionManager/SimpleMissionItemTest.cc \
        $$PWD/MissionManager/SpeedSectionTest.cc \
        $$PWD/MissionManager/StructureScanComplexItemTest.cc \
        $$PWD/MissionManager/SurveyComplexItemTest.cc \
        $$PWD/MissionManager/TransectStyleComplexItemTest.cc \
        $$PWD/MissionManager/TransectStyleComplexItemTestBase.cc \
        $$PWD/MissionManager/VisualMissionItemTest.cc \
        #$$PWD/qgcunittest/FileDialogTest.cc \
        #$$PWD/qgcunittest/MainWindowTest.cc \
        #$$PWD/qgcunittest/MessageBoxTest.cc \
        #$$PWD/AutoPilotPlugins/RadioConfigTest.cc \
        $$PWD/Vehicle/Components/ComponentInformationCacheTest.cc \
        $$PWD/Vehicle/Components/ComponentInformationTranslationTest.cc \
        $$PWD/AnalyzeView/MavlinkLogTest.cc \
        $$PWD/qgcunittest/MultiSignalSpy.cc \
        $$PWD/qgcunittest/MultiSignalSpyV2.cc \
        $$PWD/qgcunittest/UnitTest.cc \
        $$PWD/Terrain/TerrainQueryTest.cc \
        $$PWD/UnitTestList.cc \
        $$PWD/Vehicle/FTPManagerTest.cc \
        $$PWD/Vehicle/InitialConnectTest.cc \
        $$PWD/Vehicle/RequestMessageTest.cc \
        $$PWD/Vehicle/SendMavCommandWithHandlerTest.cc \
        $$PWD/Vehicle/SendMavCommandWithSignallingTest.cc \
        $$PWD/Vehicle/VehicleLinkManagerTest.cc \
}

