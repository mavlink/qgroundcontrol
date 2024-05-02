find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(FlightMapItemsQml
	SOURCES
		CameraTriggerIndicator.qml
		CustomMapItems.qml
		MissionItemIndicator.qml
		MissionItemIndicatorDrag.qml
		MissionItemView.qml
		MissionLineView.qml
		PlanMapItems.qml
		PolygonEditor.qml
		ProximityRadarMapView.qml
		SplitIndicator.qml
		VehicleMapItem.qml
)
