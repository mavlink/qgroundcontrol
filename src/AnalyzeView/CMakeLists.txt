find_package(Qt6 REQUIRED COMPONENTS Core Charts Gui Qml Widgets)

qt_add_library(AnalyzeView STATIC
	ExifParser.cc
	ExifParser.h
	GeoTagController.cc
	GeoTagController.h
	LogDownloadController.cc
	LogDownloadController.h
	MavlinkConsoleController.cc
	MavlinkConsoleController.h
	MAVLinkInspectorController.cc
	MAVLinkInspectorController.h
	PX4LogParser.cc
	PX4LogParser.h
	ULogParser.cc
	ULogParser.h
)

add_custom_target(AnalyzeViewQml
	SOURCES
		AnalyzePage.qml
		AnalyzeView.qml
		GeoTagPage.qml
		LogDownloadPage.qml
		MavlinkConsolePage.qml
		MAVLinkInspectorPage.qml
		VibrationPage.qml
)

target_link_libraries(AnalyzeView
	PRIVATE
		Qt6::Charts
		Qt6::Gui
		Qt6::Qml
		Qt6::Widgets
		FactSystem
		QGC
		QGCLocation
		Settings
		Utilities
		Vehicle
	PUBLIC
		Qt6::Core
		QmlControls
)

target_include_directories(AnalyzeView PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
