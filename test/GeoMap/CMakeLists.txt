# ============================================================================
# GeoMap Unit Tests
# Tests for the experimental GeoMap engine backend
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CheckerboardTextureDataTest.cc
        CheckerboardTextureDataTest.h
        GeoSceneTest.cc
        GeoSceneTest.h
        GeoMapCameraTest.cc
        GeoMapCameraTest.h
        HeightSourceTest.cc
        HeightSourceTest.h
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
        TerrariumHeightSourceTest.cc
        TerrariumHeightSourceTest.h
        TileImageSourceTest.cc
        TileImageSourceTest.h
        TileMathTest.cc
        TileMathTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(CheckerboardTextureDataTest LABELS Unit)
add_qgc_test(GeoSceneTest LABELS Unit)
add_qgc_test(GeoMapCameraTest LABELS Unit)
add_qgc_test(HeightSourceTest LABELS Unit)
add_qgc_test(PatchGeometryTest LABELS Unit)
add_qgc_test(PatchTextureDataTest LABELS Unit)
add_qgc_test(SurfaceAnalysisTest LABELS Unit)
add_qgc_test(SurfaceModelTest LABELS Unit)
add_qgc_test(SurfacePatchImageryTest LABELS Integration)
add_qgc_test(SurfacePatchModelTest LABELS Unit)
add_qgc_test(TerrariumHeightSourceTest LABELS Integration Terrain)
add_qgc_test(TileImageSourceTest LABELS Integration)
add_qgc_test(TileMathTest LABELS Unit)
