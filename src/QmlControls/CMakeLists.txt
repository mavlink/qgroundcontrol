find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Location Positioning Qml QmlIntegration Quick Widgets)

qt_add_library(QmlControls STATIC
	AppMessages.cc
	AppMessages.h
	CustomAction.cc
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
    QGCQGeoCoordinate.cc
    QGCQGeoCoordinate.h
	QGCImageProvider.cc
	QGCImageProvider.h
	QGCFenceCircle.cc
	QGCFenceCircle.h
	QGCFencePolygon.cc
	QGCFencePolygon.h
	QGCMapCircle.cc
	QGCMapCircle.h
	QGCMapPolygon.cc
	QGCMapPolygon.h
	QGCMapPolyline.cc
	QGCMapPolyline.h
	QGCMapPalette.cc
	QGCMapPalette.h
	QGCPalette.cc
	QGCPalette.h
	QGroundControlQmlGlobal.cc
	QGroundControlQmlGlobal.h
	QmlObjectListItem.cc
	QmlObjectListItem.h
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
)

target_link_libraries(QmlControls
	PRIVATE
		Qt6::Concurrent
		Qt6::Location
        FirmwarePlugin
        Geo
        GPS
        MockLink
        Vehicle
	PUBLIC
		Qt6::Core
		Qt6::Gui
		Qt6::Positioning
		Qt6::QmlIntegration
		Qt6::Quick
		ADSB
        AirLink
        API
        Comms
        FactControls
		FactSystem
		MAVLink
		MissionManager
        PositionManager
		QGC
		QGCLocation
		Settings
        Terrain
		Utilities
		UTMSP
        VideoManager
)

target_include_directories(QmlControls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# qt_add_qml_module(QmlControls
#     URI QGroundControl.Controls
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES
#     	arrow-down.png
#     	checkbox-check.svg
#     	EditPositionDialog.FactMetaData.json
#     	RCToParamDialog.FactMetaData.json
#     OUTPUT_TARGETS QmlControls_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
# 		QtQuick
# 		QtQuick.Controls
# 		QtQuick.Window
# 		QtQuick.Dialogs
# 		QtQuick.Layouts
# 		Qt.labs.platform
# 		QGroundControl
# 		QGroundControl.Controllers
# 		QGroundControl.FactControls
# 		QGroundControl.Palette
# 		QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )

############# QmlPalette

# find_package(Qt6 REQUIRED COMPONENTS Core Gui QmlIntegration)

# qt_add_library(QmlPalette STATIC
# 	QGCMapPalette.cc
# 	QGCMapPalette.h
# 	QGCPalette.cc
# 	QGCPalette.h
# )

# target_link_libraries(QmlPalette
# 	PRIVATE
# 		API
# 		QGC
# 	PUBLIC
# 		Qt6::Core
# 		Qt6::Gui
# 		Qt6::QmlIntegration
# )

# target_include_directories(QmlPalette PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(QmlPalette
#     URI QGroundControl.Palette
#     RESOURCE_PREFIX "/"
#     VERSION 1.0
#     OUTPUT_TARGETS QmlPalette_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     DEPENDENCIES
#         QtCore
#         QtGui
# )
