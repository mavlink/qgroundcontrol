
add_library(FlightMap
	Widgets/ValuesWidgetController.cc
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

