# ============================================================================
# GeoMap Unit Tests
# Tests for the experimental GeoMap engine backend
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        CheckerboardTextureDataTest.cc
        CheckerboardTextureDataTest.h
        ElevationTilePyramidTest.cc
        ElevationTilePyramidTest.h
        FenceWallGeometryTest.cc
        FenceWallGeometryTest.h
        FlightPathGeometryTest.cc
        FlightPathGeometryTest.h
        GeoSceneTest.cc
        GeoSceneTest.h
        GeoMapCameraTest.cc
        GeoMapCameraTest.h
        GeoMapItemTest.cc
        GeoMapItemTest.h
        GeoMapProjectedPathTest.cc
        GeoMapProjectedPathTest.h
        HeightFieldTest.cc
        HeightFieldTest.h
        HeightSourceTest.cc
        HeightSourceTest.h
        PaperPlaneGeometryTest.cc
        PaperPlaneGeometryTest.h
        PatchGeometryTest.cc
        PatchGeometryTest.h
        PatchTextureDataTest.cc
        PatchTextureDataTest.h
        SurfaceAnalysisTest.cc
        SurfaceAnalysisTest.h
        SurfaceModelTest.cc
        SurfaceModelTest.h
        SurfacePatchImageryTest.cc
        SurfacePatchImageryTest.h
        SurfacePatchModelTest.cc
        SurfacePatchModelTest.h
        TerrariumTileFetcherTest.cc
        TerrariumTileFetcherTest.h
        TileImageSourceTest.cc
        TileImageSourceTest.h
        TileMathTest.cc
        TileMathTest.h
)

# The tests reuse file-local helper names (kViewport, kCenter, setupCamera, ...), so they cannot share unity units.
file(GLOB _geomap_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
set_source_files_properties(
    ${_geomap_test_sources}
    TARGET_DIRECTORY
    QGCUnitTests
    PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(CheckerboardTextureDataTest LABELS Unit)
add_qgc_test(ElevationTilePyramidTest LABELS Unit)
add_qgc_test(FenceWallGeometryTest LABELS Unit)
add_qgc_test(FlightPathGeometryTest LABELS Unit)
add_qgc_test(GeoSceneTest LABELS Unit)
add_qgc_test(GeoMapCameraTest LABELS Unit)
add_qgc_test(GeoMapItemTest LABELS Unit)
add_qgc_test(GeoMapProjectedPathTest LABELS Unit)
add_qgc_test(HeightFieldTest LABELS Unit)
add_qgc_test(HeightSourceTest LABELS Unit)
add_qgc_test(PaperPlaneGeometryTest LABELS Unit)
add_qgc_test(PatchGeometryTest LABELS Unit)
add_qgc_test(PatchTextureDataTest LABELS Unit)
add_qgc_test(SurfaceAnalysisTest LABELS Unit)
add_qgc_test(SurfaceModelTest LABELS Unit)
add_qgc_test(SurfacePatchImageryTest LABELS Integration)
add_qgc_test(SurfacePatchModelTest LABELS Unit)
add_qgc_test(TerrariumTileFetcherTest LABELS Integration Terrain)
add_qgc_test(TileImageSourceTest LABELS Integration)
add_qgc_test(TileMathTest LABELS Unit)
