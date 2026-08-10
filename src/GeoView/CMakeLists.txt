# ============================================================================
# GeoView Module
# Experimental custom 2D/3D map engine (preview feature)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        HeightSource.cc
        HeightSource.h
        SurfaceModel.cc
        SurfaceModel.h
        TerrainHeightSource.cc
        TerrainHeightSource.h
        TileImageSource.cc
        TileImageSource.h
        TileMath.cc
        TileMath.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(GeoViewModule STATIC)

qt_add_qml_module(GeoViewModule
    URI QGroundControl.GeoView
    VERSION 1.0
    RESOURCE_PREFIX /qml
    IMPORTS
        QtQuick
        QtQuick3D
    SOURCES
        CheckerboardTextureData.cc
        CheckerboardTextureData.h
        GeoScene.cc
        GeoScene.h
        GeoViewCamera.cc
        GeoViewCamera.h
        PatchGeometry.cc
        PatchGeometry.h
        PatchTextureData.cc
        PatchTextureData.h
        SurfacePatchModel.cc
        SurfacePatchModel.h
    QML_FILES
        GeoView.qml
)

target_link_libraries(GeoViewModule
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
        Qt6::Qml
        Qt6::Quick3D
)

target_include_directories(GeoViewModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
