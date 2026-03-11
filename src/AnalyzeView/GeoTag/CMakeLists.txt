# ============================================================================
# GeoTag Module
# Image geotagging from flight logs
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        DataFlashParser.cc
        DataFlashParser.h
        ExifParser.cc
        ExifParser.h
        GeoTagController.cc
        GeoTagController.h
        GeoTagData.h
        GeoTagImageModel.cc
        GeoTagImageModel.h
        ULogParser.cc
        ULogParser.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# QML Module for GeoTag UI
# ----------------------------------------------------------------------------
qt_add_library(GeoTagModule STATIC)

qt_add_qml_module(GeoTagModule
    URI QGroundControl.AnalyzeView.GeoTag
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        GeoTagPage.qml
    RESOURCES
        GeoTagIcon.svg
    NO_PLUGIN
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeoTagModule)
