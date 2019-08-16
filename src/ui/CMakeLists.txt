add_subdirectory(preferences)
add_subdirectory(toolbar)

add_library(ui

	MAVLinkDecoder.cc
	MultiVehicleDockWidget.cc
	QGCPluginHost.cc

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	MAVLinkDecoder.h
	MultiVehicleDockWidget.h
	QGCMapRCToParamDialog.h
	QGCPluginHost.h

	# UI
	MultiVehicleDockWidget.ui
	QGCMapRCToParamDialog.ui
	QGCPluginHost.ui
	QMap3D.ui
)

add_custom_target(uiQml
SOURCES
	AppSettings.qml
	ExitWithErrorWindow.qml
	MainRootWindow.qml
)

target_link_libraries(ui
	PUBLIC
		qgc
)

target_include_directories(ui
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}
	PRIVATE
		linechart
	)

