# ============================================================================
# GeoView Unit Tests
# Tests for the experimental GeoView map engine backend
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CheckerboardTextureDataTest.cc
        CheckerboardTextureDataTest.h
        GeoViewCameraTest.cc
        GeoViewCameraTest.h
        HeightSourceTest.cc
        HeightSourceTest.h
        PatchGeometryTest.cc
        PatchGeometryTest.h
        SurfaceModelTest.cc
        SurfaceModelTest.h
        SurfacePatchModelTest.cc
        SurfacePatchModelTest.h
        TileMathTest.cc
        TileMathTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(CheckerboardTextureDataTest LABELS Unit)
add_qgc_test(GeoViewCameraTest LABELS Unit)
add_qgc_test(HeightSourceTest LABELS Unit)
add_qgc_test(PatchGeometryTest LABELS Unit)
add_qgc_test(SurfaceModelTest LABELS Unit)
add_qgc_test(SurfacePatchModelTest LABELS Unit)
add_qgc_test(TileMathTest LABELS Unit)
