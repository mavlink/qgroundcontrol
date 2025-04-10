if(NOT QGC_VIEWER3D)
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CityMapGeometry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CityMapGeometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/earcut.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OsmParser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/OsmParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/OsmParserThread.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/OsmParserThread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DQmlBackend.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DQmlBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DQmlVariableTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTerrainGeometry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTerrainGeometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTerrainTexture.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTerrainTexture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTileQuery.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTileQuery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTileReply.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DTileReply.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DUtils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DUtils.h
    CACHE INTERNAL "" FORCE
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Qt6::Quick3D
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

set(QGC_DEFINES
    ${QGC_DEFINES}
    QGC_VIEWER3D
    CACHE INTERNAL "" FORCE
)

# file(GLOB_RECURSE QML_SOURCES RELATIVE Viewer3DQml Viewer3DQml/*.qml)
# file(GLOB_RECURSE QML_RESOURCES RELATIVE Viewer3DQml Viewer3DQml/*.mesh)
# qt_add_qml_module(QGC
#     URI QGroundControl.Viewer3D
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     OUTPUT_TARGETS Viewer3D_targets
#     RESOURCES
#       Images/city_3d_map_icon.svg
#       SampleOsmMap/map_sim_small.osm
#       Shaders/earthMaterial.frag
#       Shaders/earthMaterial.vert
#       ${QML_RESOURCES}
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtQuick.Dialogs
#       QtQuick.Layouts
#       QtQuick3D
#       QtPositioning

#       QGroundControl
#       QGroundControl.Controls
#       QGroundControl.Controllers
#       QGroundControl.FactSystem
#       QGroundControl.FlightDisplay
#       QGroundControl.FlightMap
#       QGroundControl.Palette
#       QGroundControl.ScreenTools
#       QGroundControl.Vehicle

#       QGroundControl.Viewer3D
#       Viewer3D.Models3D
# )
