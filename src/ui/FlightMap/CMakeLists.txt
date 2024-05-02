add_subdirectory(MapItems)
add_subdirectory(Widgets)

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FlightMap STATIC)

add_custom_target(FlightMapQml
	FlightMap.qml
	MapScale.qml
	QGCVideoBackground.qml
)
