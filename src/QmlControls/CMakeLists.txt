find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Location Positioning Qml Quick Widgets)

qt_add_library(QmlControls STATIC
	AppMessages.cc
	AppMessages.h
	CustomAction.h
	CustomActionManager.cc
	CustomActionManager.h
	EditPositionDialogController.cc
	EditPositionDialogController.h
	FactValueGrid.cc
	FactValueGrid.h
	FlightPathSegment.cc
	FlightPathSegment.h
	HorizontalFactValueGrid.cc
	HorizontalFactValueGrid.h
	InstrumentValueData.cc
	InstrumentValueData.h
	ParameterEditorController.cc
	ParameterEditorController.h
	QGCFileDialogController.cc
	QGCFileDialogController.h
	QGCGeoBoundingCube.cc
	QGCGeoBoundingCube.h
	QGCImageProvider.cc
	QGCImageProvider.h
	QGroundControlQmlGlobal.cc
	QGroundControlQmlGlobal.h
	QmlObjectListModel.cc
	QmlObjectListModel.h
	QmlUnitsConversion.h
	RCChannelMonitorController.cc
	RCChannelMonitorController.h
	RCToParamDialogController.cc
	RCToParamDialogController.h
	ScreenToolsController.cc
	ScreenToolsController.h
	TerrainProfile.cc
	TerrainProfile.h
	ToolStripAction.cc
	ToolStripAction.h
	ToolStripActionList.cc
	ToolStripActionList.h
    QGCMapPalette.cc
    QGCMapPalette.h
    QGCPalette.cc
    QGCPalette.h
)

target_link_libraries(QmlControls
	PRIVATE
		Qt6::Concurrent
		Qt6::Location
		Qt6::Widgets
        api
        FirmwarePlugin
        Geo
        gps
        MockLink
		Settings
	PUBLIC
		Qt6::Core
		Qt6::Gui
		Qt6::Positioning
		Qt6::Quick
		ADSB
        AirLink
        comm
        FactControls
		FactSystem
		qgc
        Terrain
		Utilities
		UTMSP
        Vehicle
)

target_include_directories(QmlControls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
file(GLOB QML_RESOURCES_SVG RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.svg)
file(GLOB QML_RESOURCES_PNG RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.png)
qt_add_qml_module(QmlControls
    URI QmlControls
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
    RESOURCES
    	"${QML_RESOURCES_SVG}"
    	"${QML_RESOURCES_PNG}"
    OUTPUT_TARGETS QmlControls_targets
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
