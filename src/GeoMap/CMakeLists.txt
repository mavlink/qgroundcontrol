# ============================================================================
# GeoMap Module
# Experimental custom 2D/3D map engine (preview feature)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        BilinearUV.h
        ElevationTilePyramid.cc
        ElevationTilePyramid.h
        HeightField.cc
        HeightField.h
        HeightSource.cc
        HeightSource.h
        PatchSampler.cc
        PatchSampler.h
        SurfaceAnalysis.cc
        SurfaceAnalysis.h
        SurfaceModel.cc
        SurfaceModel.h
        TerrariumTileFetcher.cc
        TerrariumTileFetcher.h
        TileImageSource.cc
        TileImageSource.h
        TileMath.cc
        TileMath.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(GeoMapModule STATIC)

qt_add_qml_module(GeoMapModule
    URI QGroundControl.GeoMap
    VERSION 1.0
    RESOURCE_PREFIX /qml
    IMPORTS
        QtQuick
        QtQuick3D
        QtQuick3D.Helpers
        QtQuick.Shapes
    SOURCES
        CheckerboardTextureData.cc
        CheckerboardTextureData.h
        FlightPathGeometry.cc
        FlightPathGeometry.h
        GeoScene.cc
        GeoScene.h
        GeoMapCamera.cc
        GeoMapCamera.h
        GeoMapItem.cc
        GeoMapItem.h
        GeoMapProjectedPath.cc
        GeoMapProjectedPath.h
        PaperPlaneGeometry.cc
        PaperPlaneGeometry.h
        PatchGeometry.cc
        PatchGeometry.h
        PatchTextureData.cc
        PatchTextureData.h
        SurfacePatchModel.cc
        SurfacePatchModel.h
    QML_FILES
        GeoMap.qml
        GeoMapFlightPath.qml
        GeoMapFWLandingPatternVisual.qml
        GeoMapLandingPatternVisuals.qml
        GeoMapMissionArrow.qml
        GeoMapMissionDirectionArrows.qml
        GeoMapMissionHeightBadge.qml
        GeoMapMissionItems.qml
        GeoMapMissionLabel.qml
        GeoMapMissionPath.qml
        GeoMapPin.qml
        GeoMapSegmentArrow.qml
        GeoMapVehicleItem.qml
        GeoMapVTOLLandingPatternVisual.qml
        GeoMapWaypointItem.qml
    RESOURCES
        shaders/flightpath.frag
        shaders/flightpath.vert
)

target_link_libraries(GeoMapModule
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
        Qt6::Qml
        Qt6::Quick
        Qt6::Quick3D
    PRIVATE
        QGCLogging
)

target_include_directories(GeoMapModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
