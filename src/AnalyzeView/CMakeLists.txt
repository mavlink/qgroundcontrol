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

target_link_libraries(AnalyzeView
	PRIVATE
		Qt6::Charts
        Qt6::Gui
		Qt6::Qml
        Qt6::Widgets
		FactSystem
		qgc
		QGCLocation
		Settings
		Utilities
		Vehicle
	PUBLIC
		Qt6::Core
                AutoPilotPlugins
		QmlControls
)

target_include_directories(AnalyzeView PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
file(GLOB QML_RESOURCES_SVG RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.svg)
file(GLOB QML_RESOURCES_PNG RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
qt_add_qml_module(AnalyzeView
    URI AnalyzeView
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    RESOURCES
    	"${QML_RESOURCES_SVG}"
    	"${QML_RESOURCES_PNG}"
    OUTPUT_TARGETS AnalyzeView_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
		QtCharts
    	QtLocation
    	QtMultimedia
		QtPositioning
	    QtQml
		QtQml.Models
		QtQuick
		QtQuick.Controls
		QtQuick.Dialogs
		QtQuick.Extras
		QtQuick.Layouts
		QtQuick.Window
		QGroundControl
		QGroundControl.Controllers
		QGroundControl.Controls
		QGroundControl.FactControls
		QGroundControl.FactSystem
		QGroundControl.FlightDisplay
		QGroundControl.FlightMap
		QGroundControl.Palette
		QGroundControl.ScreenTools
		QGroundControl.ShapeFileHelper
		QGroundControl.UTMSP
		QGroundControl.Vehicle
)
