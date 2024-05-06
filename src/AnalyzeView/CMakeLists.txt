find_package(Qt6 REQUIRED COMPONENTS Core Charts Gui Qml QmlIntegration Widgets)

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
		Qt6::QmlIntegration
		MAVLink
		QmlControls
)

target_include_directories(AnalyzeView INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(AnalyzeView
#     URI QGroundControl.AnalyzeView
#     VERSION 1.0
#     QML_FILES
# 		AnalyzePage.qml
# 		AnalyzeView.qml
# 		GeoTagPage.qml
# 		LogDownloadPage.qml
# 		MavlinkConsolePage.qml
# 		MAVLinkInspectorPage.qml
# 		VibrationPage.qml
#     RESOURCES
#     	FloatingWindow.svg
#     	GeoTagIcon.svg
#     	LogDownloadIcon.svg
#     	MavlinkConsoleIcon.svg
#     	MAVLinkInspector.svg
#     	VibrationPageIcon.png
#     OUTPUT_TARGETS AnalyzeView_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
# 		QtQuick
# 		QtQuick.Controls
# 		QtQuick.Dialogs
# 		QtQuick.Layouts
# 		QtQuick.Window
# 		QtCharts
# 		Qt.labs.qmlmodels
# 		QGroundControl
# 		QGroundControl.Palette
# 		QGroundControl.Controls
# 		QGroundControl.Controllers
# 		QGroundControl.FactSystem
# 		QGroundControl.FactControls
# 		QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )
