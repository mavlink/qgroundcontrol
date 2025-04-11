set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AppMessages.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AppMessages.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkAction.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkActionManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkActionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EditPositionDialogController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/EditPositionDialogController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactValueGrid.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactValueGrid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightPathSegment.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightPathSegment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HorizontalFactValueGrid.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/HorizontalFactValueGrid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InstrumentValueData.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/InstrumentValueData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterEditorController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterEditorController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFileDialogController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFileDialogController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCGeoBoundingCube.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCGeoBoundingCube.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCQGeoCoordinate.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCQGeoCoordinate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCImageProvider.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCImageProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFenceCircle.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFenceCircle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFencePolygon.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCFencePolygon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapCircle.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapCircle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolygon.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolygon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolyline.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPolyline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPalette.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapPalette.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCPalette.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCPalette.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGroundControlQmlGlobal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGroundControlQmlGlobal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QmlObjectListModel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QmlObjectListModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QmlUnitsConversion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RCChannelMonitorController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RCChannelMonitorController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RCToParamDialogController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RCToParamDialogController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScreenToolsController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ScreenToolsController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainProfile.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainProfile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolStripAction.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolStripAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolStripActionList.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolStripActionList.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# qt_add_qml_module(QmlControls
#     URI QGroundControl.Controls
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES
#       arrow-down.png
#       checkbox-check.svg
#       EditPositionDialog.FactMetaData.json
#       RCToParamDialog.FactMetaData.json
#     OUTPUT_TARGETS QmlControls_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtQuick.Window
#       QtQuick.Dialogs
#       QtQuick.Layouts
#       Qt.labs.platform
#       QGroundControl
#       QGroundControl.Controllers
#       QGroundControl.FactControls
#       QGroundControl.Palette
#       QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )
