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

add_qgc_test(CityMapGeometryTest LABELS Unit Viewer3D)
add_qgc_test(OsmParserTest LABELS Unit Viewer3D)
add_qgc_test(OsmParserThreadTest LABELS Unit Viewer3D RESOURCE_LOCK TempFiles)
add_qgc_test(Viewer3DTerrainGeometryTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DTileQueryTest LABELS Unit Viewer3D)
add_qgc_test(GeoCoordinateTypeTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DInstancingTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DManagerTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DMapProviderTest LABELS Unit Viewer3D)
