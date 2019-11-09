
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		CameraCalcTest.cc
		CameraSectionTest.cc
		CorridorScanComplexItemTest.cc
		FWLandingPatternTest.cc
		MissionCommandTreeTest.cc
		MissionControllerManagerTest.cc
		MissionControllerTest.cc
		MissionItemTest.cc
		MissionManagerTest.cc
		MissionSettingsTest.cc
		PlanMasterControllerTest.cc
		QGCMapPolygonTest.cc
		QGCMapPolylineTest.cc
		SectionTest.cc
		SimpleMissionItemTest.cc
		SpeedSectionTest.cc
		StructureScanComplexItemTest.cc
		SurveyComplexItemTest.cc
		TransectStyleComplexItemTest.cc
		VisualMissionItemTest.cc
	)
endif()

add_library(MissionManager
	CameraCalc.cc
	CameraSection.cc
	CameraSpec.cc
	ComplexMissionItem.cc
	CorridorScanComplexItem.cc
	CorridorScanPlanCreator.cc
	CustomPlanCreator.cc
	FixedWingLandingComplexItem.cc
	GeoFenceController.cc
	GeoFenceManager.cc
	KML.cc
	MissionCommandList.cc
	MissionCommandTree.cc
	MissionCommandUIInfo.cc
	MissionController.cc
	MissionItem.cc
	MissionManager.cc
	MissionSettingsItem.cc
	PlanCreator.cc
	PlanElementController.cc
	PlanManager.cc
	PlanMasterController.cc
	QGCFenceCircle.cc
	QGCFencePolygon.cc
	QGCMapCircle.cc
	QGCMapPolygon.cc
	QGCMapPolyline.cc
	RallyPoint.cc
	RallyPointController.cc
	RallyPointManager.cc
	SimpleMissionItem.cc
	SpeedSection.cc
	StructureScanComplexItem.cc
	StructureScanPlanCreator.cc
	SurveyComplexItem.cc
	SurveyPlanCreator.cc
	TransectStyleComplexItem.cc
	VisualMissionItem.cc

	Section.h # shouldn't be listed here, but isn't named properly for AUTOMOC

	${EXTRA_SRC}
)

add_custom_Target(MisionManagerQml
SOURCES
	QGCMapCircleVisuals.qml  
	QGCMapPolygonVisuals.qml  
	QGCMapPolylineVisuals.qml
)

target_link_libraries(MissionManager
	PUBLIC
		Qt5::Xml
                qgc
	PRIVATE
		qgc
)

target_include_directories(MissionManager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
