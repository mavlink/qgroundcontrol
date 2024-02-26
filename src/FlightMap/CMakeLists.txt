
add_subdirectory(MapItems)
add_subdirectory(Widgets)

qt_add_library(FlightMap STATIC
	#Widgets/ValuesWidgetController.cc
)

target_link_libraries(FlightMap
	PRIVATE
		qgc
)

target_include_directories(FlightMap
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	INTERFACE
		Widgets
	)

add_custom_target(FligthMapQml
	FlightMap.qml
	MapScale.qml
	QGCVideoBackground.qml
)
