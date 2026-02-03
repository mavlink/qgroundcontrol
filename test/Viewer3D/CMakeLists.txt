# ============================================================================
# Viewer3D Unit Tests
# Tests for 3D viewer geometry, parsing, and coordinate utilities
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GeoCoordinateTypeTest.cc
        GeoCoordinateTypeTest.h
        Viewer3DManagerTest.cc
        Viewer3DManagerTest.h
        Viewer3DMapProviderTest.cc
        Viewer3DMapProviderTest.h
        Viewer3DTileQueryTest.cc
        Viewer3DTileQueryTest.h
        Providers/Osm/CityMapGeometryTest.cc
        Providers/Osm/CityMapGeometryTest.h
        Providers/Osm/OsmParserTest.cc
        Providers/Osm/OsmParserTest.h
        Providers/Osm/OsmParserThreadTest.cc
        Providers/Osm/OsmParserThreadTest.h
        Viewer3DInstancingTest.cc
        Viewer3DInstancingTest.h
        Viewer3DTerrainGeometryTest.cc
        Viewer3DTerrainGeometryTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Providers/Osm)

qt_add_resources(${CMAKE_PROJECT_NAME} "Viewer3DTest_res"
    PREFIX "/unittest"
    BASE Providers/Osm
    FILES
        Providers/Osm/map_sim_small.osm
        Providers/Osm/test_buildings.osm
)
