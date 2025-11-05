# ============================================================================
# Geo Utilities Unit Tests
# Tests for geographic calculations, shapefile parsing, and KML handling
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GeoTest.cc
        GeoTest.h
        ShapeTest.cc
        ShapeTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "Test_Utilities_Geo_res"
    PREFIX "/unittest"
    FILES
        pline.dbf
        pline.prj
        pline.shp
        pline.shx
        polygon.dbf
        polygon.prj
        polygon.shp
        polygon.shx
        polyline.kml
        polygon.kml
)
