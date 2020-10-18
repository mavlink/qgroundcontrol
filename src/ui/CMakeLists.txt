add_subdirectory(preferences)
add_subdirectory(toolbar)

add_library(ui
	QGCPluginHost.cc
	QGCPluginHost.h

	# UI
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

