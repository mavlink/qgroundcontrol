find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Viewer3D STATIC)

find_package(Qt6 COMPONENTS OPTIONAL_COMPONENTS Quick3D)
cmake_dependent_option(QGC_DISABLE_VIEWER3D "Disable Viewer3D" OFF "Qt6Quick3D_FOUND" ON) # This removes QtQuick3D which is GPL licensed
if(NOT QGC_DISABLE_VIEWER3D)
    message(STATUS "Viewer3D is Initialized")

    find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Positioning Qml Quick3D Xml)

    target_sources(Viewer3D
        PRIVATE
            CityMapGeometry.cc
            CityMapGeometry.h
            earcut.hpp
            OsmParser.cc
            OsmParser.h
            OsmParserThread.cc
            OsmParserThread.h
            Viewer3DManager.cc
            Viewer3DManager.h
            Viewer3DQmlBackend.cc
            Viewer3DQmlBackend.h
            Viewer3DQmlVariableTypes.h
            Viewer3DTerrainGeometry.cc
            Viewer3DTerrainGeometry.h
            Viewer3DTerrainTexture.cc
            Viewer3DTerrainTexture.h
            Viewer3DTileQuery.cc
            Viewer3DTileQuery.h
            Viewer3DTileReply.cc
            Viewer3DTileReply.h
            Viewer3DUtils.cc
            Viewer3DUtils.h
    )

    target_link_libraries(Viewer3D
        PRIVATE
            Qt6::Network
            QGC
            QGCLocation
            Settings
            Vehicle
        PUBLIC
            Qt6::Core
            Qt6::Gui
            Qt6::Positioning
            Qt6::Quick3D
            Qt6::Xml
    )

    target_include_directories(Viewer3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    target_compile_definitions(Viewer3D PUBLIC QGC_DISABLE_VIEWER3D)
endif()

# file(GLOB_RECURSE QML_SOURCES RELATIVE Viewer3DQml Viewer3DQml/*.qml)
# file(GLOB_RECURSE QML_RESOURCES RELATIVE Viewer3DQml Viewer3DQml/*.mesh)
# qt_add_qml_module(Viewer3D
#     URI QGroundControl.Viewer3D
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     OUTPUT_TARGETS Viewer3D_targets
#     RESOURCES
#         Images/city_3d_map_icon.svg
#         SampleOsmMap/map_sim_small.osm
#         Shaders/earthMaterial.frag
#         Shaders/earthMaterial.vert
#         ${QML_RESOURCES}
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts
#         QtQuick3D
#         QtPositioning

#         QGroundControl
#         QGroundControl.Controls
#         QGroundControl.Controllers
#         QGroundControl.FactSystem
#         QGroundControl.FlightDisplay
#         QGroundControl.FlightMap
#         QGroundControl.Palette
#         QGroundControl.ScreenTools
#         QGroundControl.Vehicle

#         QGroundControl.Viewer3D
#         Viewer3D.Models3D
# )
